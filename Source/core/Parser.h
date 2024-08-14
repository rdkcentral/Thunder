/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2020 Metrological
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef __PARSER_H
#define __PARSER_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Enumerate.h"
#include "KeyValue.h"
#include "Number.h"
#include "Optional.h"
#include "Portability.h"
#include "Range.h"

namespace Thunder {
	namespace Core {
		template <typename TEXTTERMINATOR, typename HANDLER>
		class ParserType {
		private:
			using ThisClass = ParserType<TEXTTERMINATOR, HANDLER>;

		public:
			ParserType() = delete;
			ParserType(const ParserType<TEXTTERMINATOR, HANDLER>&) = delete;
			ParserType<TEXTTERMINATOR, HANDLER>& operator=(const ParserType<TEXTTERMINATOR, HANDLER>&) = delete;

			enum ParseState {
				SKIP_WHITESPACE = 0x001,
				WORD_CAPTURE = 0x002,
				QUOTED = 0x004,
				ESCAPED = 0x008,
				FLUSH_LINE = 0x010,
				EXTERNALPASS = 0x020,
				UPPERCASE = 0x040,
				LOWERCASE = 0x080,
				SPLITCHAR = 0x100,
				PARSESTOP = 0x200,
				WAS_QUOTED = 0x400,
				SINGLE_QUOTED = 0x800
			};

			ParserType(HANDLER& parent)
				: _state(SKIP_WHITESPACE | WORD_CAPTURE)
				, _byteCounter(0)
				, _buffer()
				, _parent(parent)
				, _splitChar(0)
				, _terminator()
			{
			}
			~ParserType() = default;

		public:
			inline void Reset()
			{
				_state = SKIP_WHITESPACE | WORD_CAPTURE;
				_byteCounter = 0;
				_splitChar = 0;
			}
			inline uint32_t Position() const {
				return (_byteCounter);
			}
			inline void CollectWord()
			{
				_state = WORD_CAPTURE | (_state & (~(UPPERCASE | LOWERCASE | SPLITCHAR)));
			}
			inline void CollectWord(TCHAR splitChar)
			{
				_state = WORD_CAPTURE | (_state & (~(UPPERCASE | LOWERCASE))) | SPLITCHAR;
				_splitChar = splitChar;
			}
			inline void CollectWord(const ParseState caseState)
			{
				_state = WORD_CAPTURE | (_state & (~(UPPERCASE | LOWERCASE | SPLITCHAR))) | ((UPPERCASE | LOWERCASE) & caseState);
			}
			inline void CollectWord(TCHAR splitChar, const ParseState caseState)
			{
				_state = WORD_CAPTURE | (_state & (~(UPPERCASE | LOWERCASE))) | ((UPPERCASE | LOWERCASE) & caseState) | SPLITCHAR;
				_splitChar = splitChar;
			}
			inline void CollectLine()
			{
				_state = (_state & (~(WORD_CAPTURE | UPPERCASE | LOWERCASE | SPLITCHAR)));
			}
			inline void CollectLine(const ParseState caseState)
			{
				_state = (_state & (~(WORD_CAPTURE | UPPERCASE | LOWERCASE | SPLITCHAR))) | ((UPPERCASE | LOWERCASE) & caseState);
			}
			inline void FlushLine()
			{
				_state |= FLUSH_LINE;
			}
			inline void PassThrough(const uint32_t passThroughBytes)
			{
				_state |= EXTERNALPASS | SKIP_WHITESPACE;
				_byteCounter = passThroughBytes;
			}
			uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength)
			{
				uint16_t current = 0;

				while (current < maxLength) {
					// Pass through if requested..

					while (((_state & EXTERNALPASS) != 0) && (current < maxLength)) {
						uint16_t passOn = (static_cast<uint32_t>(maxLength - current) > _byteCounter ? static_cast<uint16_t>(_byteCounter) : (maxLength - current));

						if (_parent.Parse(&stream[current], passOn)) {

							// It might be chunked passthrough
							_byteCounter -= passOn;
							current += passOn;

							if (_byteCounter == 0) {
								_state &= (~EXTERNALPASS);
								_parent.EndOfPassThrough();
							}
						}
						else {
							_parent.EndOfPassThrough();
							_state = PARSESTOP;
							_byteCounter = 0;
							break;
						}
					}

					if ((_state & PARSESTOP) != 0) {
						break;
					}

					// Skip a line if required...
					while (((_state & FLUSH_LINE) != 0) && (current < maxLength)) {
						int8_t terminated = _terminator.IsTerminated(stream[current]);

						if ((terminated & 0x80) == 0x80) {
							_state &= (~FLUSH_LINE);
							_state |= (SKIP_WHITESPACE);
							_parent.EndOfLine();
						}
						current++;
					}

					// If we are still in whitespace skipping mode, skipp all white spaces...
					while (((_state & SKIP_WHITESPACE) != 0) && (current < maxLength)) {
						if ((stream[current] == ' ') || (stream[current] == '\t')) {
							current++;
						}
						else {
							_state &= (~SKIP_WHITESPACE);
						}
						_byteCounter++;
					}

					while (((_state & SKIP_WHITESPACE) == 0) && (current < maxLength)) {
						TCHAR character = ((_state & UPPERCASE) != 0 ? toupper(stream[current]) : ((_state & LOWERCASE) != 0 ? tolower(stream[current]) : stream[current]));

						if ((_state & ESCAPED) == ESCAPED) {
							// NO need to do any validation, this one is part of the stream to capture.
							_state &= (~ESCAPED);
							_buffer += character;
						}
						else if (character == '\\') {
							_state |= ESCAPED;
							_buffer += character;
						}
						else if ((character == '\"') || (character == '\'')) {
							if ((_state & QUOTED) == QUOTED) {
								// Depending on the quote type, we just add it, or we close the quotation
								if ((((_state & SINGLE_QUOTED) != 0) && (character == '\'')) ||
									(((_state & SINGLE_QUOTED) == 0) && (character == '\"'))) {
									_state &= (~QUOTED);
									_state |= WAS_QUOTED;
								}
								else {
									_buffer += character;
								}
							}
							else {
								if (__Complete(_buffer, character) == true) {
									_parent.Parse(_buffer, false);
									_buffer.clear();
								}
								if (_buffer.size() == 0) {
									_state |= (QUOTED) | (character == '\'' ? SINGLE_QUOTED : 0);
								}
								else {
									_buffer += character;
								}
							}
						}
						else if ((_state & QUOTED) == QUOTED) {
							_buffer += character;
						}
						else {
							int8_t terminated = _terminator.IsTerminated(character);

							if ((terminated & 0x80) == 0x80) {
								_byteCounter = 0;
								_parent.Parse(_buffer, ((_state & WAS_QUOTED) == WAS_QUOTED));
								_state &= (~(WAS_QUOTED | SINGLE_QUOTED));
								_parent.EndOfLine();
								_state |= SKIP_WHITESPACE;
								_buffer.clear();
							}
							else if ((terminated & 0x40) != 0x40) {
								if ((stream[current] == ' ') || (stream[current] == '\t') || (stream[current] == '\r') || (__Complete(_buffer, character))) {
									if ((_state & WORD_CAPTURE) == WORD_CAPTURE) {
										if (_splitChar == character) {
											_buffer += character;
										}
										if ((_buffer.empty() == false) || ((_state & WAS_QUOTED) == WAS_QUOTED)) {
											_parent.Parse(_buffer, ((_state & WAS_QUOTED) == WAS_QUOTED));
											_buffer.clear();
											_state &= (~(WAS_QUOTED|SINGLE_QUOTED));
										}

										if ((_splitChar != character) && (character != ' ') && (character != '\t') && (character != '\r')) {
											__Complete(_buffer, character);
											_buffer = string(&character, 1);
										}
										else {
											_state |= SKIP_WHITESPACE;
										}
									}
									else {
										_buffer += character;
									}
									_byteCounter++;

								}
								else {
									_buffer += character;
									_byteCounter++;
								}
							}
						}

						// We handled this character..
						current++;
					}
				}

				return (current);
			}

		private:
			// -----------------------------------------------------
			// Check for Clear method on Object
			// -----------------------------------------------------
            IS_MEMBER_AVAILABLE(Complete, hasComplete);

			template < typename T=HANDLER>
			inline typename Core::TypeTraits::enable_if<hasComplete<T, bool, const string&, const TCHAR>::value, bool>::type
				__Complete(const string& buffer, const TCHAR character)
			{
				return (_parent.Complete(buffer, character));
			}

			template < typename T=HANDLER>
			inline typename Core::TypeTraits::enable_if<!hasComplete<T, bool, const string&, const TCHAR>::value, bool>::type
				__Complete(const string& /* buffer */, const TCHAR character)
			{
				return (((_state & SPLITCHAR) == SPLITCHAR) && (character == _splitChar));
			}

		private:
			uint16_t _state;
			uint32_t _byteCounter;
			string _buffer;
			HANDLER& _parent;
			TCHAR _splitChar;
			TEXTTERMINATOR _terminator;
		};

		class EXTERNAL TextParser : public TextFragment {
		public:
			TextParser() = delete;
			TextParser(const TextParser&) = delete;
			TextParser& operator=(const TextParser&) = delete;

			TextParser(const TextFragment& input)
				: TextFragment(input)
			{
			}
			~TextParser() = default;

		public:
			void ReadText(OptionalType<TextFragment>& result, const TCHAR delimiters[]);

			inline void Skip(const uint32_t positions)
			{
				Forward(positions);
			}

			inline void Skip(const TCHAR characters[])
			{
				Forward(ForwardSkip(characters));
			}

			inline void Find(const TCHAR characters[])
			{
				Forward(ForwardFind(characters));
			}

			inline void SkipWhiteSpace()
			{
				Skip(_T("\t "));
			}

			inline void SkipLine()
			{
				Jump(_T("\n\r\0"));
			}

			template <const bool CASESENSITIVE>
			bool Validate(const TextFragment& comparison)
			{
				uint32_t marker = ForwardSkip(_T("\t "));

				bool result = (((CASESENSITIVE == true) && (comparison == TextFragment(*this, marker, comparison.Length()))) || ((CASESENSITIVE == false) && (comparison.EqualText(TextFragment(*this, marker, comparison.Length())))));

				if (result == true) {
					Forward(marker + comparison.Length());
				}

				return (result);
			}

			template <typename NUMBER, const bool SIGNED>
			void ReadNumber(OptionalType<NUMBER>& result, const NumberBase type = BASE_DECIMAL)
			{
				NUMBER value;

				uint32_t marker = ForwardSkip(_T("\t "));

				// Read the number..
				uint32_t readChars = NumberType<NUMBER, SIGNED>::Convert(&(Data()[marker]), Length() - marker, value, type);

				// See if we read a number (did we progress??)
				if (readChars != 0) {
					Forward(marker + readChars);
					result = value;
				}
			}

			template <typename ENUMERATE, const bool CASESENSITIVE>
			void ReadEnumerate(OptionalType<ENUMERATE>& result, const TCHAR* delimiters)
			{
				OptionalType<TextFragment> textEnumerate;

				uint32_t marker = ForwardSkip(_T("\t "));

				uint32_t progressed = ReadText(delimiters, marker);

				// See if we read a number (did we progress??)
				if (progressed != marker) {
					EnumerateType<ENUMERATE> enumerate(TextFragment(*this, marker, progressed - marker), CASESENSITIVE);

					if (enumerate.IsSet()) {
						// We have an enumerate, forward..
						Skip(progressed);

						result = enumerate.Value();
					}
				}
			}

			template <const TCHAR DIVIDER, const bool CASESENSITIVE>
			void ReadKeyValuePair(OptionalType<TextKeyValueType<CASESENSITIVE, TextFragment>>& result, const TCHAR delimiters[])
			{
				uint32_t readBytes = ForwardSkip(_T("\t "));
				uint32_t marker = readBytes;

				const TCHAR* line = &(Data()[readBytes]);

				// Walk on till we reach the divider
				while ((readBytes < Length()) && (*line != DIVIDER) && (_tcschr(delimiters, *line) == nullptr)) {
					line++;
					readBytes++;
				}

				// time to remember the key..
				TextFragment key = TextFragment(*this, marker, readBytes - marker);
				OptionalType<TextFragment> value;

				// And did we end up on the divider...
				if (*line == DIVIDER) {
					++readBytes;

					marker = readBytes;

					// Now we found the divider, lets find the value..
					readBytes = ReadText(delimiters, readBytes);

					if (readBytes != marker) {
						value = TextFragment(*this, marker, readBytes - marker);
					}
				}

				if (key.Length() != 0) {
					// All succeeded, move forward.
					Skip(readBytes);

					result = TextKeyValueType<CASESENSITIVE, Core::TextFragment>(key, value);
				}
			}

			template <const TCHAR DIVIDER, typename NUMBER, const bool SIGNED, const bool BEGININC, const bool ENDINC>
			void ReadRange(OptionalType<RangeType<NUMBER, BEGININC, ENDINC>>& result, const NumberBase type = BASE_DECIMAL)
			{
				NUMBER minimum;
				NUMBER maximum;

				// Read a number, it might be in..
				uint32_t readBytes = NumberType<NUMBER, SIGNED>::Convert(Data(), Length(), minimum, type);

				if (readBytes == 0) {
					minimum = NumberType<NUMBER, SIGNED>::Min();
				}

				// Now skip all white space if it is behind..
				readBytes = ForwardSkip(_T("\t "), readBytes);

				// Now check if we have the DIVIDER.
				if (Data()[readBytes] == DIVIDER) {
					readBytes++;

					// Now time to get the maximum.
					uint32_t handled = NumberType<NUMBER, SIGNED>::Convert(&(Data()[readBytes]), Length() - readBytes, maximum, type);

					if (handled == 0) {
						maximum = NumberType<NUMBER, SIGNED>::Max();
					}

					// It succeeded, skip this part..
					Skip(readBytes + handled);

					result = RangeType<NUMBER, BEGININC, ENDINC>(minimum, maximum);
				}
			}

		private:
			inline void Jump(const TCHAR characters[])
			{
				uint32_t index = ForwardFind(characters);

				if (index >= Length()) {
					Forward(Length());
				}
				else {
					Forward(ForwardSkip(characters, index));
				}
			}

			uint32_t ReadText(const TCHAR delimiters[], const uint32_t offset = 0) const;
		};

		class EXTERNAL PathParser {
		public:
			PathParser() = delete;
			PathParser(const PathParser&) = delete;
			PathParser& operator=(const PathParser&) = delete;

			inline PathParser(const TextFragment& input)
			{
				Parse(input);
			}
			~PathParser() = default;

		public:
			inline const OptionalType<TCHAR>& Drive() const
			{
				return (m_Drive);
			}

			inline const OptionalType<TextFragment>& Path() const
			{
				return (m_Path);
			}

			inline const OptionalType<TextFragment>& FileName() const
			{
				return (m_FileName);
			}

			inline const OptionalType<TextFragment>& BaseFileName() const
			{
				return (m_BaseFileName);
			}

			inline const OptionalType<TextFragment>& Extension() const
			{
				return (m_Extension);
			}

		private:
			void Parse(const TextFragment& input);

		private:
			OptionalType<TCHAR> m_Drive;
			OptionalType<TextFragment> m_Path;
			OptionalType<TextFragment> m_FileName;
			OptionalType<TextFragment> m_BaseFileName;
			OptionalType<TextFragment> m_Extension;
		};
	}
} // namespace Core

#endif // __PARSER_H
