# distutils: language=c++
# cython: c_string_type=unicode, c_string_encoding=utf8

import typing
from cython.operator cimport dereference as deref, postincrement as postinc
from libcpp.algorithm cimport lower_bound
from .engine cimport *
from collections import namedtuple
from .labels import Labels


cdef char* eType_to_str(eType t) except NULL:
	"""Convert entity type from enum to string."""
	if t == NonRelevant:
		return 'NonRelevant'
	elif t == Concept:
		return 'Concept'
	elif t == Relation:
		return 'Relation'
	elif t == PathRelevant:
		return 'PathRelevant'
	raise ValueError(f'Entity type {t} is unrecognized.')


cdef char* aType_to_str(Attribute t) except NULL:
	"""Convert attribute type from enum to string."""
	if t == Negation:
		return 'Negation'
	elif t == DateTime:
		return 'DateTime'
	elif t == PositiveSentiment:
		return 'PositiveSentiment'
	elif t == NegativeSentiment:
		return 'NegativeSentiment'
	elif t == EntityVector:
		return 'EntityVector'
	elif t == Frequency:
		return 'Frequency'
	elif t == Duration:
		return 'Duration'
	elif t == Measurement:
		return 'Measurement'
	elif t == Certainty:
		return 'Certainty'
	elif t == Generic1:
		return 'Generic1'
	elif t == Generic2:
		return 'Generic2'
	elif t == Generic3:
		return 'Generic3'

	raise ValueError(f'Attribute type {t} is unrecognized.')


Entry = namedtuple('Entry', ['literal', 'label'])

cdef class UserDictionary(object):
	"""A class that represents a user dictionary"""
	cdef CPPUserDictionary user_dictionary

	# keep track of entries in a readable form
	cdef list _entries

	def __cinit__(self):
		"""Initialize the underlying C++ iKnowUserDictionary class"""
		self.user_dictionary = CPPUserDictionary()

	def __init__(self, load_entries=None):
		self._entries = []
		self.add_all(load_entries)

	@property
	def entries(self) -> list:
		return self._entries

	def clear(self) -> None:
		"""Clear the User Dictionary object"""
		self.user_dictionary.clear()
		self._entries = []

	def add_all(self, load_entries=None) -> None:
		"""Append the contents of load_entries to this User Dictionary """
		# load one by one so they also trigger the C++ side
		if isinstance(load_entries, UserDictionary):
			load_entries = load_entries.entries
		if load_entries != None:
			for e in load_entries:
				if isinstance(e, Entry):
					self.add_label(e.literal, e.label)
				else:
					self.add_label(e['literal'], e['label'])

	def add_label(self, str literal: typing.Text, str UdctLabel: typing.Text) -> None:
		"""Add a custom user dictionary label."""
		# capture pseudo-labels for sentence end/noend
		if UdctLabel == Labels.SENTENCE_END:
			self.add_sent_end_condition(literal, True)
		elif UdctLabel == Labels.SENTENCE_NO_END:
			self.add_sent_end_condition(literal, False)
		elif self.user_dictionary.addLabel(literal, UdctLabel) == 0:
			self._entries.append(Entry(literal, UdctLabel))
		else:
			raise ValueError(f'User Dictionary Label {UdctLabel!r} is unknown.')

	def add_certainty_level(self, str literal: typing.Text, int level) -> None:
		"""Specify a token certainty level"""
		if self.user_dictionary.addCertaintyLevel(literal, level) == 0:
			self._entries.append(Entry(literal, Labels.CERTAINTY))
		else:
			raise ValueError(f"Certainty value {level!r} out of range")

	def add_sent_end_condition(self, str literal: typing.Text, cpp_bool bSentenceEnd: bool = True) -> None:
		"""Add a sentence end condition."""
		self.user_dictionary.addSEndCondition(literal, bSentenceEnd)
		if bSentenceEnd:
			self._entries.append(Entry(literal, Labels.SENTENCE_END))
		else:
			self._entries.append(Entry(literal, Labels.SENTENCE_NO_END))

	def add_concept(self, str literal: typing.Text) -> None:
		"""Add a concept term."""
		self.user_dictionary.addConceptTerm(literal)
		self._entries.append(Entry(literal, Labels.CONCEPT))

	def add_relation(self, str literal: typing.Text) -> None:
		"""Add a relation term."""
		self.user_dictionary.addRelationTerm(literal)
		self._entries.append(Entry(literal, Labels.RELATION))

	def add_non_relevant(self, str literal: typing.Text) -> None:
		"""Add a non relevant term."""
		self.user_dictionary.addNonrelevantTerm(literal)
		self._entries.append(Entry(literal, Labels.NONRELEVANT))

	def add_negation(self, str literal: typing.Text) -> None:
		"""Add a negation term."""
		self.user_dictionary.addNegationTerm(literal)
		self._entries.append(Entry(literal, Labels.NEGATION))

	def add_positive_sentiment(self, str literal: typing.Text) -> None:
		"""Add a positive sentiment term."""
		self.user_dictionary.addPositiveSentimentTerm(literal)
		self._entries.append(Entry(literal, Labels.POS_SENTIMENT))

	def add_negative_sentiment(self, str literal: typing.Text) -> None:
		"""Add a negative sentiment term."""
		self.user_dictionary.addNegativeSentimentTerm(literal)
		self._entries.append(Entry(literal, Labels.NEG_SENTIMENT))

	def add_unit(self, str literal: typing.Text) -> None:
		"""Add a unit term."""
		self.user_dictionary.addUnitTerm(literal)
		self._entries.append(Entry(literal, Labels.UNIT))

	def add_number(self, str literal: typing.Text) -> None:
		"""Add a number term."""
		self.user_dictionary.addNumberTerm(literal)
		self._entries.append(Entry(literal, Labels.NUMBER))

	def add_time(self, str literal: typing.Text) -> None:
		"""Add a time term."""
		self.user_dictionary.addTimeTerm(literal)
		self._entries.append(Entry(literal, Labels.TIME))

	def add_generic1(self, str literal: typing.Text) -> None:
		"""Add generic1 label."""
		self.user_dictionary.addGeneric1(literal)
		self._entries.append(Entry(literal, Labels.GENERIC1))

	def add_generic2(self, str literal: typing.Text) -> None:
		"""Add generic2 label."""
		self.user_dictionary.addGeneric2(literal)
		self._entries.append(Entry(literal, Labels.GENERIC2))

	def add_generic3(self, str literal: typing.Text) -> None:
		"""Add generic3 label."""
		self.user_dictionary.addGeneric3(literal)
		self._entries.append(Entry(literal, Labels.GENERIC3))


cdef class iKnowEngine:
	"""A class that represents an instance of the iKnow Natural Language
	Processing engine. iKnow is a library for Natural Language Processing that
	identifies entities (phrases) and their semantic context in natural language
	text in English, German, Dutch, French, Spanish, Portuguese, Swedish,
	Russian, Ukrainian, Czech and Japanese.

	Index some text by calling the index() method. After this method completes,
	results are stored in the m_index attribute. If applicable, linguistic trace
	information is stored in the m_traces attribute."""
	cdef CPPiKnowEngine engine
	cdef vector[size_t] index_shift  # shift indices to calculate surrogate pairs

	def __cinit__(self):
		"""Initialize the underlying C++ iKnowEngine class"""
		self.engine = CPPiKnowEngine()

	cdef size_t idx_shift(self, size_t index):
		"""Return a corrected index to account for the fact that the C++ engine
		uses UCS2 surrogate pairs (2x16bit chars), while Python indexes all
		chars equally."""
		return index - (lower_bound(self.index_shift.begin(), self.index_shift.end(), index) - self.index_shift.begin())

	@staticmethod
	def get_languages_set() -> typing.Set[typing.Text]:
		"""Return the set of supported languages."""
		return CPPiKnowEngine.GetLanguagesSet()

	@staticmethod
	def normalize_text(str text_source: typing.Text, str language: typing.Text, cpp_bool bUserDct: bool = False, cpp_bool bLowerCase: bool = True, cpp_bool bStripPunct: bool = True) -> typing.Text:
		"""Normalize the text_source."""
		return CPPiKnowEngine.NormalizeText(text_source, language, bUserDct, bLowerCase, bStripPunct)

	@staticmethod
	def IdentifyLanguage(str text_source: typing.Text) -> typing.Tuple[typing.Text]:
		"""Identify the language of the text source"""
		cdef double certainty = 0.0
		language = CPPiKnowEngine.IdentifyLanguage(text_source, certainty)
		return (language,str(float(certainty)))

	def index(self, str text_source: typing.Text, str language: typing.Text, cpp_bool traces: bool = False, str detect_language_at: typing.Text = "sentence") -> None:
		"""Index the text in text_source with a given language. Supported
		languages are given by get_languages_set(). After indexing, results are
		stored in the m_index attribute. The traces argument is optional and is
		False by default. If traces is True, then the linguistic trace
		information is stored in the m_traces attribute."""

		# replace '|' separators
		language = language.replace("|",",") # replace '|' separator with ',' the c++ engine uses comma separators...

		# expand if all languages are requested: '*'
		if language == '*': # select all languages
			language = ''
			for lang in self.get_languages_set(): # reconstruct language parameter
				if lang == "ja": # skip Japanese in multilingual request
					continue
				if len(language) != 0:
					language += ','
				language += lang

		# check for valid languages
		language_list = language.split(",")
		for single_language in language_list:
			if single_language not in self.get_languages_set():
				raise ValueError(f'Language {single_language!r} is not supported.')

		# set ALI mode (detect_language_at = [ "document" | "sentence" ])
		if detect_language_at == 'document': # switch to document(source) mode
			self.engine.setALIonSourceLevel()
		else:
			self.engine.setALIonSentenceLevel()

		# determine the Python indices of characters that the C++ engine will
		# convert into surrogate pairs
		self.index_shift.clear()
		cdef size_t idx = 0
		for x in text_source:
			if ord(x) > 2**16:
				self.index_shift.push_back(idx)
				idx += 2
			else:
				idx += 1

		return self.engine.index(text_source, language, traces)

	def load_user_dictionary(self, UserDictionary udct) -> None:
		"""Load User Dictionary"""
		return self.engine.loadUserDictionary(udct.user_dictionary)

	def unload_user_dictionary(self) -> None:
		"""Unload User Dictioanry"""
		return self.engine.unloadUserDictionary()

	@property
	def _m_index_raw(self):
		"""Raw representation of the index following Cython default type
		coercions. For debug use only."""
		return self.engine.m_index

	@property
	def m_traces(self) -> typing.List[typing.Text]:
		"""The linguistic trace information, as a list of strings."""
		return self.engine.m_traces

	@property
	def m_index(self) -> typing.Dict[typing.Text, typing.Any]:
		"""The data after indexing, in dictionary form.

		m_index['sentences'] : a list of sentences in the text source after
			indexing.
		m_index['sentences'][i] : the ith sentence in the text source after
			indexing.
		m_index['sentences'][i]['entities'] : a list of text entities in the ith
			sentence after indexing.
		m_index['sentences'][i]['path'] : a list representing the path in the
			ith sentence.
		m_index['sentences'][i]['path_attributes'] : a list of semantic attribute 
			expansions in the ith sentence's path.
		m_index['sentences'][i]['sent_attributes'] : a list of attribute
			sentence markers for the ith sentence.
		m_index['proximity'] : the proximity pairs in the text source after
			indexing.
		"""
		cdef list sentences_mod = []
		cdef list entities_mod, sent_attrs_mod, path_attrs_mod
		for sentence in self.engine.m_index.sentences:
			entities_mod = []
			sent_attrs_mod = []
			path_attrs_mod = []
			# use iterator instead of for-in syntax because Entity and Path_Attribute structs don't have default
			# constructors, which are required with for-in syntax
			entity_iter = sentence.entities.begin()
			while entity_iter != sentence.entities.end():
				entities_mod.append({'type': eType_to_str(deref(entity_iter).type),
				                     'offset_start': self.idx_shift(deref(entity_iter).offset_start),
				                     'offset_stop': self.idx_shift(deref(entity_iter).offset_stop),
				                     'index': deref(entity_iter).index,
				                     'dominance_value': deref(entity_iter).dominance_value,
				                     'entity_id': deref(entity_iter).entity_id})
				postinc(entity_iter)
			sent_attr_iter = sentence.sent_attributes.begin()
			while sent_attr_iter != sentence.sent_attributes.end():
				sent_attrs_mod.append({'type': aType_to_str(deref(sent_attr_iter).type),
				                       'offset_start': self.idx_shift(deref(sent_attr_iter).offset_start),
				                       'offset_stop': self.idx_shift(deref(sent_attr_iter).offset_stop),
				                       'marker': deref(sent_attr_iter).marker,
									   'parameters': deref(sent_attr_iter).parameters,
				                       'entity_ref': deref(sent_attr_iter).entity_ref,
									   'entity_vector': deref(sent_attr_iter).entity_vector})
				postinc(sent_attr_iter)
			path_attr_iter = sentence.path_attributes.begin()
			while path_attr_iter != sentence.path_attributes.end():
				path_attrs_mod.append({'type': aType_to_str(deref(path_attr_iter).type),
									   'pos': deref(path_attr_iter).pos,
									   'span': deref(path_attr_iter).span})
				postinc(path_attr_iter)
			sentences_mod.append({'entities': entities_mod, 'sent_attributes': sent_attrs_mod, 'path': sentence.path,
			                      'path_attributes': path_attrs_mod, 'crcs' : sentence.crc_chains})
		return {'sentences': sentences_mod, 'proximity': self.engine.m_index.proximity}
