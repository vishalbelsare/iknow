#include "engine.h"
#include "UserKnowledgeBase.h"

#include <numeric>
#include <mutex>
#include <memory>

#include "Process.h"
#include "SharedMemoryKnowledgebase.h"
#include "SharedMemoryLanguagebase.h"
#include "CompiledKnowledgebase.h"
#include "CompiledLanguagebase.h"
#include "IkSummarizer.h"
#include "IkPath.h"
#include "IkIndexInput.h"
#include "IkIndexProcess.h"
#include "RegExServices.h"
#include "ali.h"

const std::set<std::string>& iKnowEngine::GetLanguagesSet(void) {
	static const std::set<std::string> iknow_languages = { "en", "de", "ru", "es", "fr", "ja", "nl", "pt", "sv", "uk", "cs" };
	return iknow_languages;
}

typedef void(*OutputFunc)(iknow::core::IkIndexOutput*, iknow::core::IkIndexDebug<TraceListType>*, void*, Stemmer*);

using namespace std;

using iknow::shell::CProcess;
using iknow::shell::SharedMemoryKnowledgebase;
using iknow::shell::CompiledKnowledgebase;
using iknow::base::String;
using iknow::base::SpaceString;
using iknow::base::Char;
using iknow::base::IkStringEncoding;
using iknow::core::IkSummarizer;
using iknow::core::IkKnowledgebase;
using iknow::core::IkIndexProcess;
using iknow::core::Sentences;
using iknow::core::IkSentence;
using iknow::core::IkMergedLexrep;
using iknow::core::MergedLexreps;
using iknow::core::IkLexrep;
using iknow::core::IkPath;
using iknow::core::FastLabelSet;
using iknow::base::PoolAllocator;
using iknow::core::path::Offsets;
using iknow::core::path::CRC;
using iknow::core::path::CRCs;

using iknowdata::Text_Source;
using iknowdata::Sent_Attribute;
using iknowdata::Entity;
using iknowdata::Path_Attribute;

struct UData
{
	UData(Text_Source::Sentences& sents, iknow::core::IkConceptProximity::ProximityPairVector_t& proximity, std::vector<std::string>& traces) :
		iknow_sentences(sents),
		iknow_proximity(proximity),
		iknow_traces(traces)
	{}

	Text_Source::Sentences& iknow_sentences; // reference to sentence information
	iknow::core::IkConceptProximity::ProximityPairVector_t& iknow_proximity; // collection of proximity information
	std::vector<std::string> &iknow_traces; // linguistic trace info, on demand
};

typedef unsigned short PropertyId;

static iknow::core::RegExServices RegExHandler; // static is fine, API blocks multiple threads...

static const String kEntityString = IkStringEncoding::UTF8ToBase("Entity");
static const String kNegationString = IkStringEncoding::UTF8ToBase("Negation");
static const String kPositiveSentimentString = IkStringEncoding::UTF8ToBase("PositiveSentiment");
static const String kNegativeSentimentString = IkStringEncoding::UTF8ToBase("NegativeSentiment");
static const String kMeasurementString = IkStringEncoding::UTF8ToBase("Measurement"); // Entity(Measurement, Value, Unit), "Value", "Unit", "Value"+"Unit"
static const String kMeasurementValueString = IkStringEncoding::UTF8ToBase("Value");
static const String kMeasurementUnitString = IkStringEncoding::UTF8ToBase("Unit");
static const String kEntityVectorTypeName = IkStringEncoding::UTF8ToBase("EntityVector");

// 
// helper function translates internal iknow::core::IkLabel::Type to iknowdata::Entity::eType
// 
static inline iknowdata::Entity::eType get_ent_type(const IkMergedLexrep *lexrep) {
	switch (lexrep->GetLexrepType()) {
	case iknow::core::IkLabel::Relation: return iknowdata::Entity::Relation;
	case iknow::core::IkLabel::Concept: return iknowdata::Entity::Concept;
	case iknow::core::IkLabel::PathRelevant: return iknowdata::Entity::PathRelevant;
	default: return iknowdata::Entity::NonRelevant;
	}
}

static void iKnowEngineOutputCallback(iknow::core::IkIndexOutput* data, iknow::core::IkIndexDebug<TraceListType>* debug, void* userdata, Stemmer* stemmer)
{
	UData udata = *((UData*)userdata);

	bool bIsIdeographic = data->IsJP();
	const String lexrep_separator = (bIsIdeographic ? String() : SpaceString()); // no separator char for Japanese
	const iknow::base::Char *pText = data->GetTextPointer(); // Get original text pointer

	typedef std::map<const IkMergedLexrep*, std::pair<unsigned short, unsigned short> > mapLexrep2Entity_type;
	mapLexrep2Entity_type mapLexrep2Entity; // map lexreps to entities.

	for (Sentences::iterator i = data->SentencesBegin(); i != data->SentencesEnd(); ++i) {
		IkSentence* sentence = &(*i);
		const IkKnowledgebase* kb = sentence->GetLexrepsBegin()->LexrepsBegin()->GetKnowledgebase(); // KB does not change in a sentence.
		RegExHandler.swich_kb(kb); // Switch to the correct KB

		iknowdata::Sentence sentence_data;
		for (MergedLexreps::const_iterator j = sentence->GetLexrepsBegin(); j != sentence->GetLexrepsEnd(); ++j) { // iterate entities
			const IkMergedLexrep *lexrep = &(*j);

			// map lexrep to sentence and entity
			unsigned short idx_sentence = static_cast<unsigned short>(udata.iknow_sentences.size()); // sentence reference
			unsigned short idx_entity = static_cast<unsigned short>(sentence_data.entities.size()); // entity reference
			mapLexrep2Entity.insert(make_pair(lexrep, make_pair(idx_sentence, idx_entity))); // link ID to lexrep

			const Char* literal_start = lexrep->GetTextPointerBegin();
			const Char* literal_end = lexrep->GetTextPointerEnd();
			const size_t text_start = literal_start - pText;
			const size_t text_stop = literal_end - pText;

			// handle measurement attributes
			{
				vector<pair<String, String>> value_unit_parameters;
				value_unit_parameters.resize(1); // make at least 1 val/unit pair 

				int iPushMeasurementAttribute = 0;
				for (IkMergedLexrep::const_iterator it = lexrep->LexrepsBegin(); it != lexrep->LexrepsEnd(); it++) { // Scan for label attributes : scroll the single lexreps
					std::string a_marker = iknow::base::IkStringEncoding::BaseToUTF8(it->GetValue()); // the attribute marker, Literal representation

					bool bMarkerIsMeasure = false, bMarkerIsValue = false, bMarkerIsUnit = false;
					const size_t label_count = it->NumberOfLabels();
					for (size_t i = 0; i < label_count; ++i) {
						FastLabelSet::Index idx_label = it->GetLabelIndexAt(i);
						size_t attribute_count = kb->GetAttributeCount(idx_label); // how many attributes on this label ?
						for (size_t cnt_attribute = 0; cnt_attribute < attribute_count; cnt_attribute++) { // iterate the attributes
							iknow::core::AttributeId type_attribute = kb->GetAttributeType(idx_label, cnt_attribute);
							String name_attribute(kb->AttributeNameForId(type_attribute).data, kb->AttributeNameForId(type_attribute).size);

							if (name_attribute == kEntityString) { // found an entity attribute, read the parameters
								for (const iknow::core::AttributeId* ent_param = kb->GetAttributeParamsBegin(idx_label, cnt_attribute); ent_param != kb->GetAttributeParamsEnd(idx_label, cnt_attribute); ++ent_param) {
									String param_attribute(kb->AttributeNameForId(*ent_param).data, kb->AttributeNameForId(*ent_param).size);
									iknow::core::PropertyId id_property = kb->PropertyIdForName(param_attribute);
									iknowdata::Attribute a_type = static_cast<iknowdata::Attribute>(id_property);
									if (a_type == iknowdata::Attribute::Measurement) // measurement attribute
										bMarkerIsMeasure = true;
									if (bMarkerIsMeasure) {
										if (param_attribute == kMeasurementValueString) { // measurement value attribute type
											bMarkerIsValue = true;
										}
										if (param_attribute == kMeasurementUnitString) { // measurement unit attribute type
											bMarkerIsUnit = true;
										}
									}
								}
							}
						}
					}
					if (bMarkerIsMeasure) { // collect for merged lexrep
						if (!iPushMeasurementAttribute) {
							sentence_data.sent_attributes.push_back(Sent_Attribute(iknowdata::Attribute::Measurement, it->GetTextPointerBegin() - pText, it->GetTextPointerEnd() - pText, a_marker)); // write marker info
							sentence_data.sent_attributes.back().entity_ref = static_cast<unsigned short>(sentence_data.entities.size()); // connect sentence attribute to entity
						}
						if (iPushMeasurementAttribute > 0) { // adjust marker, except for the first.
							sentence_data.sent_attributes.back().marker_ += (bIsIdeographic ? "" : " ") + a_marker;
							sentence_data.sent_attributes.back().offset_stop_ = it->GetTextPointerEnd() - pText;
						}
						// handle value & unit
						String at_value, at_unit, at_value2, at_unit2; // measurement attribute properties
						String at_token = it->GetValue();
						if (bMarkerIsValue && bMarkerIsUnit) { // separate value & unit properties
							if (!RegExHandler.SplitValueUnit(it->GetNormalizedText(), at_value, at_unit)) {  // failed to separate
								int parts = RegExHandler.Parser2(it->GetNormalizedText(), at_value, at_unit, at_value2, at_unit2); // refined value/unit parser
								if (parts == 0) at_value = it->GetNormalizedText(); // set as Value
							}
						}
						else {
							if (bMarkerIsValue) at_value = it->GetNormalizedText();
							if (bMarkerIsUnit) at_unit = it->GetNormalizedText();
						}
						if (at_value.length()) {
							if (value_unit_parameters.back().first.empty()) { // last value is empty, store
								value_unit_parameters.back().first = at_value;
							}
							else { // create new value/unit pair
								value_unit_parameters.push_back(make_pair(at_value, String())); // store value
							}
						}
						if (at_unit.length()) {
							if (value_unit_parameters.back().second.empty()) { // no unit in last value/unit pair, write it
								value_unit_parameters.back().second = at_unit;
							}
							else { // unit exist, make a new pair with the unit
								value_unit_parameters.push_back(make_pair(String(), at_unit));
							}
						}
						if (at_value2.length()) {
							if (value_unit_parameters.back().first.empty()) { // last value is empty, store
								value_unit_parameters.back().first = at_value;
							}
							else { // create new value/unit pair
								value_unit_parameters.push_back(make_pair(at_value2, String())); // store value
							}
						}
						if (at_unit2.length()) {
							if (value_unit_parameters.back().second.empty()) { // no unit in last value/unit pair, write it
								value_unit_parameters.back().second = at_unit;
							}
							else { // unit exist, make a new pair with the unit
								value_unit_parameters.push_back(make_pair(String(), at_unit2));
							}
						}
						iPushMeasurementAttribute++;
					}
				}
				if (iPushMeasurementAttribute) { // only if measurement attributes
					// write the value/unit pairs to the sentence attribute structure
					// int idx_parameters = value_unit_parameters.size();
					int idx_parameters = 0;
					Sent_Attribute& ref = sentence_data.sent_attributes.back();
					for (auto it = value_unit_parameters.begin(); it != value_unit_parameters.end(); ++it, ++idx_parameters) {
						String& value = it->first;
						String& unit = it->second;

						if (idx_parameters < 2) { // reserved fixes space for 2 value/unit pairs
							ref.parameters_[idx_parameters].first = iknow::base::IkStringEncoding::BaseToUTF8(value);
							ref.parameters_[idx_parameters].second = iknow::base::IkStringEncoding::BaseToUTF8(unit);
						}
						else { // extra value/unit pairs
							ref.parameters_.push_back(make_pair(iknow::base::IkStringEncoding::BaseToUTF8(value), iknow::base::IkStringEncoding::BaseToUTF8(unit)));
						}
					}
				}
			}
			std::map<iknowdata::Attribute, size_t> mapLexrepAttributes;
			for (IkMergedLexrep::const_iterator it = lexrep->LexrepsBegin(); it != lexrep->LexrepsEnd(); it++) { // Scan for label attributes : scroll the single lexreps		
				std::set<iknowdata::Attribute> setTokenAttributes; // token as marker
				const size_t label_count = it->NumberOfLabels();
				for (size_t i = 0; i < label_count; ++i) {
					FastLabelSet::Index idx_label = it->GetLabelIndexAt(i);
					size_t attribute_count = kb->GetAttributeCount(idx_label); // how many attributes on this label ?
					for (size_t cnt_attribute = 0; cnt_attribute < attribute_count; cnt_attribute++) { // iterate the attributes
						iknow::core::AttributeId type_attribute = kb->GetAttributeType(idx_label, cnt_attribute);
						String name_attribute(kb->AttributeNameForId(type_attribute).data, kb->AttributeNameForId(type_attribute).size);

						if (name_attribute == kEntityString) { // found an entity attribute, read the parameters							
							iknow::core::PropertyId id_property = 0; // first parameter is property
							for (const iknow::core::AttributeId* ent_param = kb->GetAttributeParamsBegin(idx_label, cnt_attribute); ent_param != kb->GetAttributeParamsEnd(idx_label, cnt_attribute); ++ent_param) {
								String param_attribute(kb->AttributeNameForId(*ent_param).data, kb->AttributeNameForId(*ent_param).size);
								if (!id_property) 
									id_property = kb->PropertyIdForName(param_attribute);
							}
							iknowdata::Attribute a_type = static_cast<iknowdata::Attribute>(id_property);
							std::string a_marker = iknow::base::IkStringEncoding::BaseToUTF8(it->GetValue()); // the attribute marker, Literal representation
							if (it != lexrep->LexrepsBegin()) { // if token starts with " ", remove it since it belongs to the previous token
								if (a_marker[0] == ' ') { // remove first token
									size_t next_space = a_marker.find(' ', 1);
									if (next_space != string::npos) {
										string stripped(&a_marker[next_space+1]);
										a_marker = stripped;
									}
								}
							}
							if (a_type != iknowdata::Attribute::Measurement) { // measurements are handled separately
								if (mapLexrepAttributes.count(a_type)) { // continue existing
									if (setTokenAttributes.count(a_type)) // token already in.
										continue;

									size_t idxSentAttribute = mapLexrepAttributes[a_type];
									sentence_data.sent_attributes[idxSentAttribute].offset_stop_ = it->GetTextPointerEnd() - pText; // enlarge the literal
									sentence_data.sent_attributes[idxSentAttribute].marker_ += string(" ") + a_marker; // add the extra marker
								}
								else { // new
									sentence_data.sent_attributes.push_back(Sent_Attribute(a_type, it->GetTextPointerBegin() - pText, it->GetTextPointerEnd() - pText, a_marker)); // write marker info
									sentence_data.sent_attributes.back().entity_ref = static_cast<unsigned short>(sentence_data.entities.size()); // connect sentence attribute to entity
									if (id_property == IKATTCERTAINTY) { // certainty attribute, emit the certainty level
										char certainty_data = it->GetCertainty(); // certainty char, "0" to "9"
										if (certainty_data != '\0') {
											std::string certainty_value = "0";
											certainty_value[0] = certainty_data; // '0' to '9'
											Sent_Attribute& ref = sentence_data.sent_attributes.back();
											// ref.value_ = certainty_value;
											ref.parameters_[0].first = certainty_value;
										}
									}
									mapLexrepAttributes.insert(make_pair(a_type, sentence_data.sent_attributes.size()-1)); // link lexrep attribute to data structure
									setTokenAttributes.insert(a_type);
								}
							}
						}
					}
				}				
			}
			std::string index_value = IkStringEncoding::BaseToUTF8(lexrep->GetNormalizedText());
			iknow::core::IkIndexOutput::EntityId ent_id = data->GetEntityID(lexrep);
			iknowdata::Entity::eType e_type = get_ent_type(lexrep);
			sentence_data.entities.push_back(Entity(e_type, text_start, text_stop, index_value, data->GetEntityDominance(lexrep), ent_id));
		}
		// collect sentence path data
		if (bIsIdeographic) { // handle entity vectors
			// iknow::core::PropertyId entity_vector_prop_id = kb->PropertyIdForName(kEntityVectorTypeName); // entity vector property
			IkSentence::EntityVector& entity_vector = sentence->GetEntityVector();
			if (!entity_vector.empty()) { // emit entity vectors
				// Sent_Attribute::aType a_type = static_cast<Sent_Attribute::aType>(entity_vector_prop_id);

				static const String kEntityVectorTypeName = IkStringEncoding::UTF8ToBase("EntityVector");
				PropertyId entity_vector_prop_id = kb->PropertyIdForName(kEntityVectorTypeName);
				iknowdata::Attribute a_type = static_cast<iknowdata::Attribute>(entity_vector_prop_id);

				sentence_data.sent_attributes.push_back(Sent_Attribute(a_type)); // generate entity vector marker
				// sentence_data.sent_attributes.back().entity_ref = static_cast<unsigned short>(sentence_data.entities.size()); // connect sentence attribute to entity

				for (IkSentence::EntityVector::const_iterator i = entity_vector.begin(); i != entity_vector.end(); ++i) { // collect entity id's
					sentence_data.sent_attributes.back().entity_vector.push_back((unsigned short)*i);
				}
			}
		}
		{ // handle path attributes
			{	// collect path attribute expansions
				DirectOutputPaths& sent_paths = data->paths_[udata.iknow_sentences.size()]; // paths for the sentence (in fact, only one per sentence after introducing path_relevants)
				for (DirectOutputPaths::iterator it_path = sent_paths.begin(); it_path != sent_paths.end(); ++it_path) // iterate all paths (in fact, only one...)
				{
					size_t path_length = it_path->offsets.size();
					for (int i = 0; i < path_length; ++i) {
						const IkMergedLexrep* lexrep = it_path->offsets[i];
						unsigned short entity_id = mapLexrep2Entity[lexrep].second; // entity id from lexrep
						sentence_data.path.push_back(entity_id); // reference to sentence entities
					}
					DirectOutputPathAttributeMap& amap = it_path->attributes;
					for (DirectOutputPathAttributeMap::iterator it_attr = amap.begin(); it_attr != amap.end(); ++it_attr) { // iterate per attribute id
						PropertyId id_attr = it_attr->first;
						DirectOutputPathAttributes& path_attr = it_attr->second;
						for (DirectOutputPathAttributes::iterator it_path_attr = path_attr.begin(); it_path_attr != path_attr.end(); ++it_path_attr) { // iterate per attribute id path
							DirectOutputPathAttribute& pa = *it_path_attr; // single attribute id path = path attribute expansion
							PropertyId id_attr_path = pa.type; // is equal to "id_attr"
							PathOffset attr_path_begin = pa.begin; // refers to path
							PathOffset attr_path_end = pa.end; // refers to path
							// cout << id_attr_path << ":" << attr_path_begin << ":" << attr_path_end << std::endl;
							Path_Attribute path_attribute;
							path_attribute.type = static_cast<iknowdata::Attribute>(id_attr_path);
							path_attribute.pos = (unsigned short) pa.begin; // start position
							path_attribute.span = (unsigned short) (pa.begin == pa.end ? 1 : pa.end - pa.begin); // attribute expansion span, minimum = 1
							sentence_data.path_attributes.push_back(path_attribute);
						}
					}
				}
			}
		}
		udata.iknow_sentences.push_back(sentence_data); // Collect single sentence data
	}
	data->GetProximityPairVector(udata.iknow_proximity); // Proximity is document related

	if (debug) {
		const iknow::base::IkTrace<Utf8List>& trace_data = debug->GetTrace();
		udata.iknow_traces.reserve(trace_data.end() - trace_data.begin()); // reserve memory for storage vector
		for (iknow::base::IkTrace<Utf8List>::Items::const_iterator it = trace_data.begin(); it != trace_data.end(); ++it) {
			const String& key = it->first;
			const Utf8List& valueList = it->second;
			string value;
			for_each(valueList.begin(), valueList.end(), [&value](const string& item) { value += (item + ";"); });

			udata.iknow_traces.push_back(IkStringEncoding::BaseToUTF8(key) + ":" + value);
		}
	}
}

static SharedMemoryKnowledgebase *pUserDCT = NULL; // User dictionary, one per process

iKnowEngine::iKnowEngine() // Constructor
{
}

iKnowEngine::~iKnowEngine() // Destructor
{
}

//
// Utility functions...
//
vector<string> iKnowEngine::split_row(string row_text, char split)
{
	vector<std::string> split_data;
	istringstream f(row_text);
	string s;
	while (getline(f, s, split)) {
		split_data.push_back(s);
	}
	return split_data;
}
string iKnowEngine::merge_row(vector<string>& row_vector, char split)
{
	string merge;
	static const char split_string[] = { split, '\0' };
	static const string Split(split_string);
	for_each(row_vector.begin(), row_vector.end(), [&merge](string& piece) mutable { merge += (piece + Split);  });
	return merge;
}

static std::mutex mtx;           // mutex for process.IndexFunc critical section

void iKnowEngine::index(iknow::base::String& text_input, const std::string& utf8language, bool b_trace)
{
	vector<string> index_languages = split_row(utf8language, ',');
	bool b_multilingual = index_languages.size() > 1;
	map<string, std::auto_ptr<CompiledKnowledgebase>> language_kb_map;
	static iknow::ali::LanguagebaseMap language_lb_map; // collector of language bases
	iknow::ali::Languagebases().clear(); // clear loaded ALI data
	for_each(index_languages.begin(), index_languages.end(), [&language_kb_map,b_multilingual](string& lang) {
		if (GetLanguagesSet().count(lang) == 0) // language not supported
			throw ExceptionFrom<iKnowEngine>("Language not supported");
		iknow::model::RawDataPointer kb_raw_data = CompiledKnowledgebase::GetRawData(lang);
		if (!kb_raw_data) { // no kb raw data in language module
			throw ExceptionFrom<iKnowEngine>("Language:\"" + string(lang) + "\" module has no embedded model data : old stye KB used !");
		}
		language_kb_map[lang] = std::auto_ptr<CompiledKnowledgebase>(new CompiledKnowledgebase(kb_raw_data, lang));
		if (b_multilingual) { // load ALI data
			bool b_is_compiled = true;
			if (lang == "ja")
				throw ExceptionFrom<iKnowEngine>("Japanese language cannot be used in a multilingual configuration");
			iknow::ali::LanguagebaseMap::iterator i = language_lb_map.find(IkStringEncoding::UTF8ToBase(lang)); // do we have a language base for ALI ?
			if (i == language_lb_map.end()) {
				iknow::shell::FileLanguagebase my_lang_base(lang.c_str(), b_is_compiled);
				const int kRawSize = b_is_compiled ? 100 : 2000000;
				unsigned char* buf_ = new unsigned char[kRawSize];
				iknow::shell::Raw raw(buf_, kRawSize);
				iknow::shell::RawAllocator allocator(raw);
				iknow::shell::SharedMemoryLanguagebase* slb = new iknow::shell::SharedMemoryLanguagebase(allocator, my_lang_base, b_is_compiled);
				if (my_lang_base.IsCompiled()) {
					//The unique_ptr ensures the original shared memory lb is deleted when
					//we're done constructing the new compiled one, which needs its raw_data pointer.
					std::unique_ptr<iknow::shell::SharedMemoryLanguagebase> original(slb);
					slb = new iknow::shell::CompiledLanguagebase(slb, lang);
				}
				language_lb_map[IkStringEncoding::UTF8ToBase(lang)] = slb;
			}
			iknow::ali::Languagebases()[IkStringEncoding::UTF8ToBase(lang)] = language_lb_map[IkStringEncoding::UTF8ToBase(lang)];
		}
	});
	std::unique_lock<std::mutex> lck(mtx, std::defer_lock);

	m_index.sentences.clear();
	m_index.proximity.clear();
	m_traces.clear();
	UData udata(m_index.sentences, m_index.proximity, m_traces);

	CProcess::type_languageKbMap temp_map; // storage for all KB's
	for_each(language_kb_map.begin(), language_kb_map.end(), [&temp_map](std::pair<const string, std::auto_ptr<CompiledKnowledgebase>>& lang) {
		temp_map.insert(CProcess::type_languageKbMap::value_type(IkStringEncoding::UTF8ToBase(lang.first), lang.second.get()));
		});
	CProcess process(temp_map);

	lck.lock(); // critical section (exclusive access to IndexFunc by locking lck):

	process.setUserDictionary(pUserDCT);
	if (pUserDCT)
		pUserDCT->FilterInput(text_input); // rewritings can only be applied if we no longer emit text offsets

	iknow::core::IkIndexInput Input(&text_input);
	process.IndexFunc(Input, iKnowEngineOutputCallback, &udata, true, b_trace);
	lck.unlock();
}

void iKnowEngine::index(const std::string& text_source, const std::string& language, bool b_trace) {
	String text_source_ucs2(IkStringEncoding::UTF8ToBase(text_source));
	index(text_source_ucs2, language, b_trace);
}

std::string iKnowEngine::NormalizeText(const string& text_source, const std::string& language, bool bUserDct, bool bLowerCase, bool bStripPunct) {
	try {
		iknow::model::RawDataPointer kb_raw_data = CompiledKnowledgebase::GetRawData(language);
		if (!kb_raw_data) { // no kb raw data in language module
			throw ExceptionFrom<iKnowEngine>("Language:\"" + string(language) + "\" module has no embedded model data : old stye KB used !");
		}
		SharedMemoryKnowledgebase skb = kb_raw_data; //No ALI for normalization: We need a KB.
		IkKnowledgebase* kb = &skb;
		IkKnowledgebase* ud_kb = NULL;

		std::map<iknow::base::String, IkKnowledgebase const*> null_kb_map;
		IkIndexProcess process(null_kb_map);
		String output = process.NormalizeText(IkStringEncoding::UTF8ToBase(text_source), kb, ud_kb, bLowerCase, bStripPunct);
		return IkStringEncoding::BaseToUTF8(output);
	}
	catch (const std::exception& e) {
		throw ExceptionFrom<iKnowEngine>(e.what());
	}
	throw std::runtime_error("Failed to throw an exception.");
}

// Constructor
UserDictionary::UserDictionary() {
}

// Clear the User Dictionary object.
void UserDictionary::clear() {
	m_user_data.clear();
}

// Adds User Dictionary label to a lexical representation for customizing purposes
int UserDictionary::addLabel(const std::string& literal, const char* UdctLabel)
{
	string normalized = iKnowEngine::NormalizeText(literal, "en"); // normalize the literal
	if (m_user_data.addLexrepLabel(normalized, UdctLabel) == -1) return iKnowEngine::iknow_unknown_label; // add to the udct lexreps
	return 0; // OK
}

// Add User Dictionary literal rewrite, not functional.
void UserDictionary::addEntry(const std::string& literal, const string& literal_rewrite) {
	return; // we cannot rewrite input text as long as we use text offsets to annotate text.
}

// Add User Dictionary EndNoEnd. 
void UserDictionary::addSEndCondition(const std::string& literal, bool b_end)
{
	m_user_data.addSEndCondition(literal, b_end);
}

// Add Certainty level.
int UserDictionary::addCertaintyLevel(const std::string& literal, int level)
{
	if (level < 0 || level>9)
		return iKnowEngine::iknow_certainty_level_out_of_range;

	string normalized = iKnowEngine::NormalizeText(literal, "en"); // normalize the literal
	string meta = string("c=0");
	meta[2] = (char) ((int)'0'+level);
	m_user_data.addLexrepLabel(normalized, "UDCertainty", meta); // add to the udct lexreps
	return 0;
}

// Shortcut for known UD labels
void UserDictionary::addConceptTerm(const std::string& literal) {
	addLabel(literal, "UDConcept");
}
void UserDictionary::addRelationTerm(const std::string& literal) {
	addLabel(literal, "UDRelation");
}
void UserDictionary::addNonrelevantTerm(const std::string& literal) {
	addLabel(literal, "UDNonRelevant");
}
void UserDictionary::addUnitTerm(const std::string& literal) {
	addLabel(literal, "UDUnit");
}
void UserDictionary::addNumberTerm(const std::string& literal) {
	addLabel(literal, "UDNumber");
}
void UserDictionary::addTimeTerm(const std::string& literal) {
	addLabel(literal, "UDTime");
}
void UserDictionary::addNegationTerm(const std::string& literal) {
	addLabel(literal, "UDNegation");
}
void UserDictionary::addPositiveSentimentTerm(const std::string& literal) {
	addLabel(literal, "UDPosSentiment");
}
void UserDictionary::addNegativeSentimentTerm(const std::string& literal) {
	addLabel(literal, "UDNegSentiment");
}
void UserDictionary::addGeneric1(const std::string& literal) { // tag literal as generic1
	addLabel(literal, "UDGeneric1");
}
void UserDictionary::addGeneric2(const std::string& literal) { // tag literal as generic2
	addLabel(literal, "UDGeneric2");
}
void UserDictionary::addGeneric3(const std::string& literal) { // tag literal as generic3
	addLabel(literal, "UDGeneric3");
}

// Load a User Dictionary into the iKnow engine
void iKnowEngine::loadUserDictionary(UserDictionary& udct)
{
	unloadUserDictionary(); // we can only have one
	pUserDCT = new SharedMemoryKnowledgebase(udct.m_user_data.generateRAW(false));	// first use generation.
}

// Unload the User Dictionary
void iKnowEngine::unloadUserDictionary(void)
{
	if (pUserDCT != NULL) {
		delete pUserDCT;
		pUserDCT = NULL;
	}
}