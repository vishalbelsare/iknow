#pragma once

#ifdef WIN32
#ifdef IKNOW_EXPORTS
#define IKNOW_API __declspec(dllexport)
#else
#define IKNOW_API __declspec(dllimport)
#endif
#else
#define IKNOW_API
#endif

#include "UserKnowledgeBase.h" // User KB (for udct)

//
// stl library includes
//
#include <string>
#include <vector>
#include <map>

// 
// ICU dependency, engine is build with release 65-1, look for headers and binaries on github :
// https://github.com/unicode-org/icu/releases/tag/release-65-1
//
// Unzip the files in a directory, store that directory name into environment variable ICUDIR (eg: ICUDIR=C:\thirdparty\icu4c-65_1-Win64-MSVC2017)
// Dependencies file for Visual Studio 2019 solution (modules\Dependencies.props) uses this variable to set correct ICU include and library directories.
//
#include "unicode/utypes.h"
#include "unicode/unistr.h"

//
// includes from the iKnow sources : "..\engine\src, ..\base\src\headers, ..\shell\src, ..\core\src\headers"
//
#include "IkTypes.h"
#include "IkIndexInput.h"
#include "IkIndexOutput.h"

//
// Attribute Values from IRIS Public interface
//
#define IKATTNEGATION		1
#define IKATTTIME			2
#define IKATTMODIFIER		3
#define IKATTNONSEMANTIC	4
#define IKATTSENPOSITIVE	5
#define IKATTSENNEGATIVE	6
#define IKATTENTITYVECTOR	7
#define IKATTTOPIC			8
#define IKATTFREQ			9 
#define IKATTDURATION 		10
#define IKATTMEASURE		11
#define IKATTCERTAINTY		12
#define IKATTGENERIC_1		13
#define IKATTGENERIC_2		14
#define IKATTGENERIC_3		15


namespace iknowdata { // to bundle all generated data

	typedef unsigned short Entity_Ref; // reference to entity vector, max number of entities in a sentence is 1028, so unsigned short should be enough
	typedef unsigned short Attribute_Ref; // reference to sentence attribute vector, is less (or equal) Entity_Ref.

	enum /*class*/ Attribute { // Supported attributes in NLP-Fx, should be an "enum class", but Cython refuses to build
		Negation = IKATTNEGATION,
		DateTime = IKATTTIME,
		PositiveSentiment = IKATTSENPOSITIVE,
		NegativeSentiment = IKATTSENNEGATIVE,
		EntityVector = IKATTENTITYVECTOR,
		Frequency = IKATTFREQ,
		Duration = IKATTDURATION,
		Measurement = IKATTMEASURE,
		Certainty = IKATTCERTAINTY,
		Generic1 = IKATTGENERIC_1,
		Generic2 = IKATTGENERIC_2,
		Generic3 = IKATTGENERIC_3
	};
	inline std::string AttributeName(Attribute attribute) { // translate the attribute type
		switch (attribute) {
		case Attribute::Negation:			return "negation";
		case Attribute::DateTime:			return "date_time";
		case Attribute::PositiveSentiment:	return "positive_sentiment";
		case Attribute::NegativeSentiment:	return "negative_sentiment";
		case Attribute::EntityVector:		return "entity_vector";
		case Attribute::Frequency:			return "frequency";
		case Attribute::Duration:			return "duration";
		case Attribute::Measurement:		return "measurement";
		case Attribute::Certainty:			return "certainty";
		case Attribute::Generic1:			return "generic1";
		case Attribute::Generic2:			return "generic2";
		case Attribute::Generic3:			return "generic3";

		default:							return "unknown";
		}
	}
	//
	// Basic functionality of iknow indexing is splitup of text in entities : 4
	//
	struct Entity
	{
		static const size_t kNoConcept = static_cast<size_t>(-1);	// An concept entity receives a unique index in the text source, the others: kNoConcept
		enum eType { NonRelevant = 0, Concept, Relation, PathRelevant }; // The 4 base entity base types : concept, relation, path-relevant and non-relevant

		Entity(eType type, 
			size_t start, size_t stop, 
			std::string& index, 
			double dominance = 0.0, 
			size_t entity_id = kNoConcept
		) : type_(type), offset_start_(start), offset_stop_(stop), index_(index), dominance_value_(dominance), entity_id_(entity_id) {}

		eType type_; // defines the entity type
		size_t offset_start_, offset_stop_; // these refer to offsets in the text, "start" is where the textual representation starts, "stop" is where it stops.
		std::string index_; // the normalized entity textual representation, utf8 encoded
		double dominance_value_; // a dominance value for each concept in the source document is calculated, most important concepts have highest score.
		size_t entity_id_; // unique concept index in the source document, if not concept, this value equals kNoConcept

		static inline std::string TypeName(eType ent_type) { // translate the attribute type
			switch (ent_type) {
			case eType::NonRelevant:	return "NonRelevant";
			case eType::Concept:		return "Concept";
			case eType::Relation:		return "Relation";
			case eType::PathRelevant:	return "PathRelevant";

			default:					return "unknown";
			}
		}
	};
	
	struct Sent_Attribute // sentence attribute
	{
		typedef std::vector<std::pair<std::string, std::string>> Sent_Attribute_Parameters;

		Sent_Attribute(Attribute att_type,
			size_t start, size_t stop,
			std::string& marker
		) : type_(att_type), offset_start_(start), offset_stop_(stop), marker_(marker), entity_ref(0) {
			parameters_.resize(2);
		}
		
		Sent_Attribute(Attribute att_type) : // Entity Vector attributes, no markers, no corresponding offsets
			type_(att_type), offset_start_(NULL), offset_stop_(NULL), entity_ref(0) {
			parameters_.resize(2);
		}

		Attribute type_;
		size_t offset_start_, offset_stop_; // these refer to offsets in the text, "start" is where the textual representation starts, "stop" is where it stops.
		std::string marker_; // the normalized attribute textual representation, utf8 encoded
		Sent_Attribute_Parameters parameters_; // variable number of parameters, for measurement, that are value/unit pairs.

		Entity_Ref entity_ref; // reference to entity vector, max number of entities in a sentence is 1028, so unsigned short should be enough
		std::vector<Entity_Ref> entity_vector; // EntityVector, only used in Japanese
	};

	/*
	**	Path_Attribute represents the expansion of a semantic attribute (https://github.com/intersystems/iknow/wiki/Attributes)
	**  "type" : represents the attribute type
	**  "pos" : starting position in the path (a path is a vector of entity references)
	**  "span" : number of consecutive path entities 
	*/
	struct Path_Attribute // path attribute : expresses range of attributes in path
	{
		Attribute type; // attribute type
		unsigned short pos; // start position in path
		unsigned short span; // attibute span (number of path entities)
	};

	/*
	** CRC represents the Concept/Relation/Concept chains detected in the sentence.
	** "head_concept" : index (in iKnow_Entities vector) of the head concept
	** "relation" : index of the relation that links head and tail concept
	** "tail_concept" : index of the tail concept.
	*/
	struct CRC // Concept - Relation - Concept chain
	{
		CRC(std::string head_concept, std::string relation, std::string tail_concept) :
			head_token(head_concept),
			relation_token(relation),
			tail_token(tail_concept)
		{}

		std::string head_token; // head concept
		std::string relation_token;
		std::string tail_token; // tail concept
	};

	struct Sentence
	{
		typedef std::vector<Entity> Entities;
		typedef std::vector<Sent_Attribute> Sent_Attributes;
		typedef std::vector<Entity_Ref> Path;	// unsigned short indexes the Entity in the iKnow_Entities vector 
		typedef std::vector<Path_Attribute> Path_Attributes;	// expanded attributes in path 
		typedef std::vector<CRC> CRCs; // the collection of CRC's in the sentence

		Entities			entities;	// the sentence entities
		Sent_Attributes		sent_attributes;	// the sentence attributes
		Path				path;		// the sentence path
		Path_Attributes		path_attributes;	// expanded attributes in the path
		CRCs				crc_chains; // the concept-relation-concept chains

		// utility functions : return text source offsets of the sentence : start and stop.
		size_t offset_start() const { return entities.begin()->offset_start_; }
		size_t offset_stop() const { return (entities.end() - 1)->offset_stop_; }
	};

	struct Text_Source
	{
		typedef iknow::core::IkConceptProximity::ProximityPairVector_t Proximity;
		typedef std::vector<Sentence> Sentences;

		Sentences	sentences;	// All sentence data collected from the text source
		Proximity	proximity;	// Proximity data collected
	};

	typedef Text_Source::Sentences::const_iterator SentenceIterator;
	typedef Sentence::Entities::const_iterator	EntityIterator;
	typedef Sentence::Sent_Attributes::const_iterator AttributeMarkerIterator;
	typedef Sentence::Path::const_iterator PathIterator;
	typedef Sentence::Path_Attributes::const_iterator PathAttributeIterator;

}

//
// User Dictionary for customizing iKnow output
//
class IKNOW_API UserDictionary
{
public:
	UserDictionary(); // ctor

	// Clear the User Dictionary object
	void clear();

	// Tag User Dictionary label to a lexical representation for customizing purposes
	// Currently available labels are : "UDNegation", "UDPosSentiment", "UDNegSentiment", "UDConcept", "UDRelation", "UDNonRelevant", "UDUnit", "UDNumber" and "UDTime"
	// labels can be combined, separated by ';', like "UDConcept;UDTime", "UDRelation;UDNumber"
	// Returns iKnowEngine::iknow_unknown_label if an invalid label is passed as parameter.
	int addLabel(const std::string& literal, const char* UdctLabel);

	// Add User Dictionary literal rewrite, *not* functional, added for compatibility with the IRIS implementation.
	// The purpose is to rewrite like "dr." to "doctor", to aggregate similar lexical representations.
	void addEntry(const std::string& literal, const std::string& literal_rewrite);

	// Add User Dictionary EndNoEnd, enables force/suppress sentence end conditions.
	// See iKnowUnitTests::test5() for an example.
	void addSEndCondition(const std::string& literal, bool b_end = true);

	// Shortcut for known UD labels
	void addConceptTerm(const std::string& literal); // tag literal as a concept
	void addRelationTerm(const std::string& literal); // tag literal as a relation
	void addNonrelevantTerm(const std::string& literal); // tag literal as a non-relevant

	void addUnitTerm(const std::string& literal); // tag literal as a unit
	void addNumberTerm(const std::string& literal); // tag literal as a number
	void addTimeTerm(const std::string& literal); // tag literal as a time indicator
	void addNegationTerm(const std::string& literal); // tag literal as a negation
	void addPositiveSentimentTerm(const std::string& literal); // tag literal as a positive sentiment
	void addNegativeSentimentTerm(const std::string& literal); // tag literal as a negative sentiment

	void addGeneric1(const std::string& literal); // tag literal as generic1
	void addGeneric2(const std::string& literal); // tag literal as generic2
	void addGeneric3(const std::string& literal); // tag literal as generic3

	int addCertaintyLevel(const std::string& literal, int level = 0); // add a certainty level

private:
	friend class iKnowEngine;
	iknow::csvdata::UserKnowledgeBase m_user_data; // User dictionary
};

class LanguageBase;

class IKNOW_API iKnowEngine
{
public:
	enum errcodes {
		iknow_language_not_supported = -1, // unsupported language
		iknow_unknown_label = -2,	// udct addLabel : label does not exist
		iknow_certainty_level_out_of_range = -3 // certainty level must be in the 0-9 range
	};

	// returns set of supported languages
	static const std::set<std::string>& GetLanguagesSet(void);

	// Normalizer is exposed to engine clients, needed for User Dictonary, and iFind functionality
	static std::string NormalizeText(const std::string& text_source, const std::string& language, bool bUserDct = false, bool bLowerCase = true, bool bStripPunct = true);

	// Uses ALI to detect the language of the text_source, will return one of the GetLanguagesSet() identifiers, except for Japanese !
	static std::string IdentifyLanguage(const std::string& text_source, double& certainty);

	// ctor & dtor
	iKnowEngine();
	~iKnowEngine();
	
	// The main indexing function :
	//	- text_source : the text input for indexing, must be Unicode 2byte (UCS2) encoded.
	//	- the language parameter, see supported languages
	// Current limitations : 
	//	- works synchronous : the complete text_source is indexed, after return, use iKnowEngine methods to retrieve indexing information.
	//	- works single threaded : a mutex protects multithread functioning. Use multiprocess to bypass this (current) limitation.
	void index(iknow::base::String& text_source, const std::string& language, bool b_trace=false);
	
	// Wrapper for indexing function that accepts UTF-8 encoded string instead. The offsets generated will be Unicode character
	// offsets, not byte offsets in text_source.
	void index(const std::string& text_source, const std::string& language, bool b_trace=false);


	// User dictionary methods :
	//     loadUserDictionary : will load *and* activate the user dictionary object, if a previously one is active, it will be unloaded and deactivated, will throw an exception if the udct object cannot be loaded.
	//     unloadUserDictionary : will unload and deactivate the active user dictionary.
	void loadUserDictionary(UserDictionary& udct);
	void unloadUserDictionary(void);
	void setALIonSourceLevel(void) {
		m_document_level_ALI = true;
	}
	void setALIonSentenceLevel(void) {
		m_document_level_ALI = false;
	}
	iknowdata::Text_Source m_index; // this is where all iKnow indexed information is stored after calling the "index" method.
	std::vector<std::string> m_traces; // optional collection of linguistic trace info, generated if b_trace equals true

private:
	std::vector<std::string> split_row(std::string row_text, char split);
	std::string merge_row(std::vector<std::string>& row_vector, char split);

	// collector of language bases, load once, use many times
	static std::map<iknow::base::String, LanguageBase*> m_language_lb_map;
	// helper method that makes a language ready for ALI uses.
	static void add_lang_for_ALI(std::string lang);
	bool m_document_level_ALI;
};

//
// C interface talking json
//
extern "C" {
	IKNOW_API int iknow_json(const char* request, const char** response);
}
