/*
JsonTalkie - Json Talkie is intended for direct IoT communication.
Original Copyright (c) 2025 Rui Seixas Monteiro. All right reserved.
This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.
This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.
https://github.com/ruiseixasm/JsonTalkie
*/


/**
 * @file JsonMessage.hpp
 * @brief JSON message handling for Talkie communication protocol
 * 
 * This class provides efficient, memory-safe JSON message manipulation 
 * for embedded systems with constrained resources. It implements a 
 * schema-driven JSON protocol optimized for Arduino environments.
 * 
 * @warning This class does not use dynamic memory allocation.
 *          All operations are performed on fixed-size buffers.
 * 
 * @section constraints Memory Constraints
 * - Maximum buffer size: TALKIE_BUFFER_SIZE (default: 128 bytes)
 * - Maximum name length: TALKIE_NAME_LEN (default: 16 bytes including null terminator)
 * - Maximum string length: TALKIE_MAX_LEN (default: 64 bytes including null terminator)
 * 
 * @author Rui Seixas Monteiro
 * @date Created: 2026-01-15
 * @version 2.0.0
 */

#ifndef JSON_MESSAGE_HPP
#define JSON_MESSAGE_HPP

#include <Arduino.h>        // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include "TalkieCodes.hpp"

// Guaranteed memory safety, constrained / schema-driven JSON protocol
// Advisable maximum sizes:
// 		f (from / name) → 16 bytes (15 + '\0')
// 		d (description) → 64 bytes (63 + '\0')


// #define MESSAGE_DEBUG_TIMING


using LinkType			= TalkieCodes::LinkType;
using TalkerMatch 		= TalkieCodes::TalkerMatch;
using BroadcastValue 	= TalkieCodes::BroadcastValue;
using MessageValue 		= TalkieCodes::MessageValue;
using SystemValue 		= TalkieCodes::SystemValue;
using RogerValue 		= TalkieCodes::RogerValue;
using ErrorValue 		= TalkieCodes::ErrorValue;
using ValueType 		= TalkieCodes::ValueType;

// Forward declaration
class BroadcastSocket;

/**
 * @class JsonMessage
 * @brief JSON message container and manipulator for Talkie protocol
 * 
 * This class manages JSON-formatted messages with a fixed schema:
 * - Mandatory fields: m (message), b (broadcast), i (identity), f (from)
 * - Optional fields: t (to), r (roger), s (system), a (action), 0-9 (values)
 * 
 * @note All string operations are bounds-checked to prevent buffer overflows.
 */
class JsonMessage {
public:

	#ifdef MESSAGE_DEBUG_TIMING
	unsigned long _reference_time = millis();
	#endif

private:

	char _json_payload[TALKIE_BUFFER_SIZE];			///< Internal JSON buffer
	size_t _json_length = 0;						///< Current length of JSON string


    // ============================================
    // GENERIC METHODS (Parsing utilities)
    // ============================================

    /**
     * @brief Find the position of the colon for a given key
     * @param key Single character key to search for
     * @param colon_position Starting position for search (default: 4)
     * @return Position of colon, or 0 if not found
     * 
     * @note Searches for pattern: `"key":`
     */
	size_t _get_colon_position(char key, size_t colon_position = 4) const {
		for (size_t json_i = colon_position; json_i < _json_length; ++json_i) {	// 4 because it's the shortest position possible for ':'
			if (_json_payload[json_i] == ':' && _json_payload[json_i - 2] == key && _json_payload[json_i - 3] == '"' && _json_payload[json_i - 1] == '"') {
				return json_i;
			}
		}
		return 0;
	}


    /**
     * @brief Get position of value for a given key
     * @param key Single character key
     * @param colon_position Optional hint for colon position
     * @return Position of first character after colon, or 0 if not found
     */
	size_t _get_value_position(char key, size_t colon_position = 4) const {
		colon_position = _get_colon_position(key, colon_position);
		if (colon_position) {			//     01
			return colon_position + 1;	// {"k":x}
		}
		return 0;
	}


    /**
     * @brief Get position of key character
     * @param key Single character key
     * @param colon_position Optional hint for colon position
     * @return Position of key character, or 0 if not found
     */
	size_t _get_key_position(char key, size_t colon_position = 4) const {
		colon_position = _get_colon_position(key, colon_position);
		if (colon_position) {			//   210
			return colon_position - 2;	// {"k":x}
		}
		return 0;
	}


    /**
     * @brief Calculate total field length (key + value)
     * @param key Single character key
     * @param colon_position Optional hint for colon position
     * @return Total characters occupied by field (including quotes, colon, commas)
     */
	size_t _get_field_length(char key, size_t colon_position = 4) const {
		size_t field_length = 0;
		size_t json_i = _get_value_position(key, colon_position);
		if (json_i) {
			field_length = 4;	// All keys occupy 4 '"k":' chars
			ValueType value_type = _get_value_type(key, json_i - 1);
			switch (value_type) {

				case ValueType::TALKIE_VT_STRING:
					field_length += 2;	// Adds the two '"' associated to the string
					for (json_i++; json_i < _json_length && _json_payload[json_i] != '"'; json_i++) {
						field_length++;
					}
					break;
				
				case ValueType::TALKIE_VT_INTEGER:
					for (; json_i < _json_length && !(_json_payload[json_i] > '9' || _json_payload[json_i] < '0'); json_i++) {
						field_length++;
					}
					break;
				
				default: break;
			}
		}
		return field_length;
	}


	/**
     * @brief Determine value type for a key
     * @param key Single character key
     * @param colon_position Optional hint for colon position
     * @return ValueType enum indicating the type of value
     */
	ValueType _get_value_type(char key, size_t colon_position = 4) const {
		size_t json_i = _get_value_position(key, colon_position);
		if (json_i) {
			if (_json_payload[json_i] == '"') {
				for (json_i++; json_i < _json_length && _json_payload[json_i] != '"'; json_i++) {}
				if (json_i == _json_length) {
					return ValueType::TALKIE_VT_VOID;
				}
				return ValueType::TALKIE_VT_STRING;
			} else {
				while (json_i < _json_length && _json_payload[json_i] != ',' && _json_payload[json_i] != '}') {
					if (_json_payload[json_i] > '9' || _json_payload[json_i] < '0') {
						return ValueType::TALKIE_VT_OTHER;
					}
					json_i++;
				}
				if (json_i == _json_length) {
					return ValueType::TALKIE_VT_VOID;
				}
				return ValueType::TALKIE_VT_INTEGER;
			}
		}
		return ValueType::TALKIE_VT_VOID;
	}


    /**
     * @brief Extract string value for a key
     * @param key Single character key
     * @param[out] buffer Output buffer for string
     * @param size Size of output buffer (including null terminator)
     * @param colon_position Optional hint for colon position
     * @return true if successful, false if key not found, buffer too small or not a string
	 * 
	 * For sizes equal to TALKIE_NAME_LEN, the type of chars are validate accordingly their compatibility
	 * with names, meaning, [a-z,A-Z,0-9,_]
     * 
     * @warning Buffer size must include space for null terminator
     */
	bool _get_value_string(char key, char* buffer, size_t size, size_t colon_position = 4) const {
		if (buffer && size) {
			size_t json_i = _get_value_position(key, colon_position);
			if (json_i && _json_payload[json_i++] == '"') {	// Safe code, makes sure it's a string
				size_t char_j = 0;
				if (size == TALKIE_NAME_LEN) {
					while (_json_payload[json_i] != '"' && json_i < _json_length && char_j < size) {
						// Names require specific type of chars (TALKIE_NAME_LEN)
						if (_validate_name_char(_json_payload[json_i], char_j)) {
							buffer[char_j++] = _json_payload[json_i++];
						} else {
							buffer[0] = '\0';	// Safe code, no surprises
							return false;
						}
					}
				} else {
					while (_json_payload[json_i] != '"' && json_i < _json_length && char_j < size) {
						buffer[char_j++] = _json_payload[json_i++];
					}
				}
				if (char_j < size) {
					buffer[char_j] = '\0';	// Makes sure the termination char is added
					return true;
				}
			}
			buffer[0] = '\0';	// Safe code, no surprises
		}
		return false;
	}


	/**
     * @brief Extract numeric value for a key
     * @param key Single character key
     * @param colon_position Optional hint for colon position
     * @return Extracted number, or 0 if key not found or not a number
     */
	uint32_t _get_value_number(char key, size_t colon_position = 4) const {
		uint32_t json_number = 0;
		size_t json_i = _get_value_position(key, colon_position);
		if (json_i) {
			while (json_i < _json_length && !(_json_payload[json_i] > '9' || _json_payload[json_i] < '0')) {
				json_number *= 10;
				json_number += _json_payload[json_i++] - '0';
			}
		}
		return json_number;
	}


	/**
     * @brief Extract numeric value for a key
     * @param key Single character key
     * @param number Pointer to a 32 bits number to get
     * @param colon_position Optional hint for colon position
     * @return false if no valid `uint32_t` number was found
     * 
     * @note This method checks if the number is well terminated with ',' or '}'
     */
	bool _get_value_number(char key, uint32_t* number, size_t colon_position = 4) const {
		uint32_t json_number = 0;
		size_t json_i = _get_value_position(key, colon_position);
		if (json_i) {
			while (json_i < _json_length && !(_json_payload[json_i] > '9' || _json_payload[json_i] < '0')) {
				json_number *= 10;
				json_number += _json_payload[json_i++] - '0';
			}
			// Very important validation to guarantee it isn't a truncated number due to data corruption
			if (_json_payload[json_i] == ',' || _json_payload[json_i] == '}') {
				*number = json_number;
				return true;
			}
		}
		return false;
	}


    // ============================================
    // MEMBER METHODS (Modification utilities)
    // ============================================

    /**
     * @brief Reset JSON payload to the bare minimum
     * 
     * Default bare minimum message: `{}`
     */
	void _reset() {
		_json_payload[0] = '{';
		_json_payload[1] = '}';
		_json_length = 2;
	}


    /**
     * @brief Remove a key-value pair from JSON
     * @param key Key to remove
     * @param colon_position Optional hint for colon position
     * 
     * @note Also removes leading or trailing commas as needed
     */
	void _remove_field(char key, size_t colon_position = 4) {
		colon_position = _get_colon_position(key, colon_position);
		if (colon_position) {
			size_t field_position = colon_position - 3;	// All keys occupy 3 '"k":' chars to the left of the colon
			size_t field_length = _get_field_length(key, colon_position);	// Excludes possible heading ',' separation comma
			if (_json_payload[field_position - 1] == ',') {	// the heading ',' has to be removed too
				field_position--;
				field_length++;
			} else if (_json_payload[field_position + field_length] == ',') {
				field_length++;	// Changes the length only, to pick up the tailing ','
			}
			for (size_t json_i = field_position; json_i < _json_length - field_length; json_i++) {
                _json_payload[json_i] = _json_payload[json_i + field_length];
            }
			_json_length -= field_length;	// Finally updates the _json_payload full length
		}
	}


    /**
     * @brief Set numeric value for a key
     * @param key Key to set
     * @param number Numeric value
     * @param colon_position Optional hint for colon position
     * @return true if successful, false if buffer too small
     * 
     * @note If key exists, it's replaced. Otherwise, it's added before closing brace.
     */
	bool _set_value_number(char key, uint32_t number, size_t colon_position = 4) {
		colon_position = _get_colon_position(key, colon_position);
		if (colon_position) _remove_field(key, colon_position);
		// At this time there is no field key for sure, so, one can just add it right before the '}'
		size_t number_size = number_of_digits(number);
		// the usual key 4 plus the + 1 due to the ',' needed to be added to the beginning
		size_t new_length = _json_length + 1 + 4 + number_size;
		if (new_length > TALKIE_BUFFER_SIZE) {
			return false;
		}
		// Sets the key json data
		char json_key[] = ",\"k\":";
		json_key[2] = key;
		if (_json_length > 2) {
			for (size_t char_j = 0; char_j < 5; char_j++) {
				_json_payload[_json_length - 1 + char_j] = json_key[char_j];
			}
		} else if (_json_length == 2) {	// Edge case of '{}'
			new_length--;	// Has to remove the extra ',' considered above
			for (size_t char_j = 1; char_j < 5; char_j++) {
				_json_payload[_json_length - 1 + char_j - 1] = json_key[char_j];
			}
		} else {
			_reset();	// Something very wrong, needs to be reset
			return false;
		}
		if (number) {
			// To be added, it has to be from right to left
			for (size_t json_i = new_length - 2; number; json_i--) {
				_json_payload[json_i] = '0' + number % 10;
				number /= 10; // Truncates the number (does a floor)
			}
		} else {	// Regardless being 0, it also has to be added
			_json_payload[new_length - 2] = '0';
		}
		// Finally writes the last char '}'
		_json_payload[new_length - 1] = '}';
		_json_length = new_length;
		return true;
	}


    /**
     * @brief Set numeric value for a single digit field value
     * @param key Key to set
     * @param number Numeric value
     * @param colon_position Optional hint for colon position
     * @return true if successful, false if buffer too small
     * 
     * @note If key exists, the value is replaced. Otherwise, it's added before closing brace.
     */
	bool _set_value_single_digit_number(char key, uint32_t number, size_t colon_position = 4) {
		if (number < 10) {
			colon_position = _get_colon_position(key, colon_position);
			if (colon_position) {
				size_t value_position = _get_value_position(key, colon_position);
				_json_payload[value_position] = '0' + number;
			} else {
				return _set_value_number(key, number);
			}
		}
		return false;
	}


    /**
     * @brief Set string value for a key
     * @param key Key to set
     * @param in_string String value pointer (null-terminated)
     * @param size makes sure it doesn't go beyond the in_string size - 1
     * @param colon_position Optional hint for colon position
     * @return true if successful, false if buffer too small or string empty
	 * 
	 * For sizes equal to TALKIE_NAME_LEN, the type of chars are validate accordingly their compatibility
	 * with names, meaning, [a-z,A-Z,0-9,_]
     */
	bool _set_value_string(char key, const char* in_string, size_t size, size_t colon_position = 4) {
		if (in_string) {
			size_t string_length = 0;
			if (size == TALKIE_NAME_LEN) {
				for (size_t char_j = 0; in_string[char_j] != '\0' && char_j < TALKIE_BUFFER_SIZE && char_j < size; char_j++) {
					// Names require specific type of chars (TALKIE_NAME_LEN)
					if (_validate_name_char(in_string[char_j], char_j)) {
						string_length++;
					} else {
						return false;
					}
				}
			} else {
				for (size_t char_j = 0; in_string[char_j] != '\0' && char_j < TALKIE_BUFFER_SIZE && char_j < size; char_j++) {
					string_length++;
				}
			}
			// Can't go beyond the in_string size without '\0' char (last char must be present BUT not counted thus the '<')
			if (string_length < size) {
				// It can have empty strings too, so, a string_length can be 0!
				colon_position = _get_colon_position(key, colon_position);
				if (colon_position) _remove_field(key, colon_position);
				// the usual key + 4 plus + 2 for both '"' and the + 1 due to the heading ',' needed to be added
				size_t new_length = _json_length + string_length + 1 + 4 + 2;
				if (new_length > TALKIE_BUFFER_SIZE) {
					return false;
				}
				// Sets the key json data
				char json_key[] = ",\"k\":";
				json_key[2] = key;
				// string_length to position requires - 1 and + 5 for the key (at '}' position + 5)
				size_t setting_position = _json_length - 1 + 5;
				if (_json_length > 2) {
					for (size_t char_j = 0; char_j < 5; char_j++) {
						_json_payload[_json_length - 1 + char_j] = json_key[char_j];
					}
				} else if (_json_length == 2) {	// Edge case of '{}'
					new_length--;	// Has to remove the extra ',' considered above
					setting_position--;
					for (size_t char_j = 1; char_j < 5; char_j++) {
						_json_payload[_json_length - 1 + char_j - 1] = json_key[char_j];
					}
				} else {
					_reset();	// Something very wrong, needs to be reset
					return false;
				}
				// Adds the first char '"'
				_json_payload[setting_position++] = '"';
				// To be added, it has to be from right to left
				for (size_t char_j = 0; char_j < string_length; char_j++) {
					_json_payload[setting_position++] = in_string[char_j];
				}
				// Adds the second char '"'
				_json_payload[setting_position++] = '"';
				// Finally writes the last char '}'
				_json_payload[setting_position++] = '}';
				_json_length = new_length;
				return true;
			}
		}
		return false;
	}

public:

    // ============================================
    // CONSTRUCTORS AND DESTRUCTOR
    // ============================================

    /**
     * @brief Default constructor
     * 
     * Initializes with the bare minimum: `{}`
     */
	JsonMessage() {
		_reset();	// Initiate with the bare minimum
	}


    /**
     * @brief Constructor from parameters
     * @param broadcast_value The BroadcastValue of the message
     * @param message_value The MessageValue of the message
     * 
     * Initializes with mandatory parameters
     */
	JsonMessage(MessageValue message_value, BroadcastValue broadcast_value) {
		_reset();	// Initiate with the bare minimum
		set_message_value(message_value);
		set_broadcast_value(broadcast_value);
	}


    /**
     * @brief Constructor from buffer
     * @param buffer Source buffer containing JSON
     * @param length Length of buffer
     * 
     * @note If deserialization fails, resets to default message
     */
	JsonMessage(const char* buffer, size_t length) {
		if (!deserialize_buffer(buffer, length)) {
			_reset();
		}
	}


    /**
     * @brief Copy constructor
     * @param other JsonMessage to copy from
     */
	JsonMessage(const JsonMessage& other) {
		_json_length = other._json_length;
		for (size_t json_i = 0; json_i < _json_length; ++json_i) {
			_json_payload[json_i] = other._json_payload[json_i];
		}
	}


    /**
     * @brief Destructor
     */
	~JsonMessage() {
		// Does nothing
	}


    // ============================================
    // OPERATORS
    // ============================================

    /**
     * @brief Equality operator
     * @param other JsonMessage to compare with
     * @return true if JSON content is identical
     */
	bool operator==(const JsonMessage& other) const {
		if (_json_length == other._json_length) {
			for (size_t json_i = 0; json_i < _json_length; ++json_i) {
				if (_json_payload[json_i] != other._json_payload[json_i]) {
					return false;
				}
			}
			return true;
		}
		return false;
	}


    /**
     * @brief Inequality operator
     * @param other JsonMessage to compare with
     * @return true if JSON content differs
     */
	bool operator!=(const JsonMessage& other) const {
		return !(*this == other);
	}


    /**
     * @brief Assignment operator
     * @param other JsonMessage to copy from
     * @return Reference to this object
     */
    JsonMessage& operator=(const JsonMessage& other) {
        if (this == &other) return *this;

        _json_length = other._json_length;
        for (size_t i = 0; i < _json_length; ++i) {
            _json_payload[i] = other._json_payload[i];
        }
        return *this;
    }


    // ============================================
    // BASIC OPERATIONS
    // ============================================

    /**
     * @brief Calculate number of digits in an unsigned integer
     * @param number The number to analyze
     * @return Number of decimal digits (1-10)
     * 
     * @note Handles numbers from 0 to 4,294,967,295
     */
	static size_t number_of_digits(uint32_t number) {
		size_t length = 1;	// 0 has 1 digit
		while (number > 9) {
			number /= 10;
			length++;
		}
		return length;
	}


    /**
     * @brief Validates a given character to be used in names
     * @param name_char The character belonging to a name
     * @param char_j The name position of the char
	 * 
	 * Validation accordingly to their compatibility with names, meaning, [a-z,A-Z,0-9,_],
	 * where numbers can't be used as the first name char
     * 
     * @return true if the given character at the given position can be used in a name
     */
	static bool _validate_name_char(char name_char, size_t char_j) {
		if (name_char >= 'a' && name_char <= 'z') return true;
		if (name_char >= '0' && name_char <= '9') return char_j > 0;
		if (name_char >= 'A' && name_char <= 'Z') return true;
		return name_char == '_';
	}


    /**
     * @brief Get current JSON length
     * @return Length of JSON string (not including null terminator like '\0')
     */
	size_t get_length() const {
		return _json_length;
	}


    /**
     * @brief Set current JSON length
     * @return Length of JSON string (not including null terminator like '\0')
     */
	void _set_length(size_t length) {
        _json_length = length;
    }


    /**
     * @brief Handy method that allows to add single chars one by one
     * @return true if it has space for the added char
     */
	bool _append(char c) {
		if (_json_length < TALKIE_BUFFER_SIZE) {
			_json_payload[_json_length++] = c;
			return true;
		}
		return false;
	}


    /**
     * @brief Allows a read only access to the message buffer
     * @return A constant pointer to the message buffer
     */
	const char* _read_buffer() const {
		return _json_payload;
	}


    /**
     * @brief Allows a read and write access to the message buffer
     * @param length The length of the amount of data intended to be written
     * @return A pointer to the message buffer to write on, or nullptr, if `length` is
	 *         greater than `TALKIE_BUFFER_SIZE`
     */
	char* _write_buffer(size_t length = 0) {
		if (length > TALKIE_BUFFER_SIZE) return nullptr;
        return _json_payload;
    }

	
    /**
     * @brief Reset to a bare minimum message
     * 
     * Resets to: `{}`
     */
	void reset() {
		_reset();
	}


    /**
     * @brief Deserialize from buffer
     * @param buffer Source buffer
     * @param length Length of buffer
     * @return true if successful, false if buffer is null or too large
     * 
     * @warning Does not validate JSON structure
     */
	bool deserialize_buffer(const char* buffer, size_t length) {
		if (buffer && length && length <= TALKIE_BUFFER_SIZE) {
			for (size_t char_j = 0; char_j < length; ++char_j) {
				_json_payload[char_j] = buffer[char_j];
			}
			_json_length = length;
			return true;
		}
		return false;
	}


    /**
     * @brief Serialize to buffer
     * @param[out] buffer Destination buffer
     * @param size Size of destination buffer
     * @return Number of bytes written, or 0 if buffer too small
     */
	size_t serialize_json(char* buffer, size_t size) const {
		if (buffer && size >= _json_length) {
			for (size_t json_i = 0; json_i < _json_length; ++json_i) {
				buffer[json_i] = _json_payload[json_i];
			}
			return _json_length;
		}
		return 0;
	}


    /**
     * @brief Write JSON to Print interface
     * @param out Print interface (Serial, File, etc.)
     * @return true if all bytes written successfully
     */
	bool write_to(Print& out) const {
		if (_json_length) {
			return out.write(reinterpret_cast<const uint8_t*>(_json_payload), _json_length) == _json_length;
		}
		return false;
	}


    /**
     * @brief Validates the delimiters while adjusting the last `}` one if necessary
     * @return true if the message is correctly delimited inside `{}`
     */
	bool _validate_json() {
		
		// Trim trailing newline and carriage return characters or any other that isn't '}'
		while (_json_length > 18
			&& (_json_payload[_json_length - 1] != '}' || _json_payload[_json_length - 2] == '\\')) {
			_json_length--;	// Note that literals add the '\0'!
		}

		// Minimum valid length: '{"m":0,"b":0,"i":0}' = 19
		if (_json_length < 19) {
			_reset();
			return false;
		}

		if (_json_payload[0] != '{') {
			_reset();
			return false;	
		}
		return true;
	}

	
    /**
     * @brief Corrupts a single char for debugging purposes only
     * @param any_char Includes any ascii char that not 'X'
     * @param one_in How many messages per each corruption
     */
	void _corrupt_payload(bool any_char = true, uint8_t one_in = 100) {
		static bool triggered = false;
		if (micros() % one_in == 0 && get_message_value() != MessageValue::TALKIE_MSG_ERROR) {
			if (!triggered) {
				size_t corrupted_position = millis() % _json_length;
				if (any_char) {
					char corrupted_char = static_cast<char>( (micros() + millis()) % 128 );
					_json_payload[corrupted_position] = corrupted_char;
				} else {
					_json_payload[corrupted_position] = 'X';
				}
				triggered = true;
			}
		} else {
			triggered = false;
		}
	}


	/**
     * @brief This helper method generates the checksum of a given buffer content
     */
    uint16_t generate_checksum() const {	// 16-bit word and XORing
        uint16_t checksum = 0;
		if (_json_length <= TALKIE_BUFFER_SIZE) {
			for (size_t i = 0; i < _json_length; i += 2) {
				uint16_t chunk = _json_payload[i] << 8;
				if (i + 1 < _json_length) {
					chunk |= _json_payload[i + 1];
				}
				checksum ^= chunk;
			}
		}
        return checksum;
    }


    /**
     * @brief Checks if the checksum of the message matches the on in the respective field,
	 *        if not, sets the message value as `NOISE`, so, it still shall be transmitted in order
	 *        to be processed by the Talker and be returned as error to the original sender
     * @return true if it has a valid checksum
     */
	bool _validate_checksum() {
		size_t c_colon_position = _get_colon_position('c');
		uint16_t received_checksum = _get_value_number('c', c_colon_position);
		_remove_field('c', c_colon_position);
		return generate_checksum() == received_checksum;
	}


    /**
     * @brief Generates a new message checksum and inserts it in the message
     * @return true if it had space to insert the checksum field
     */
	bool _insert_checksum() {
		_remove_field('c');	// Starts by clearing any pre existent checksum (NO surprises or miss receives)
		uint16_t checksum = generate_checksum();
		return _set_value_number('c', checksum);
	}

	
    /**
     * @brief Tries to reconstruct a corrupt message
     */
	bool _try_to_reconstruct() {
		bool repeated_keys = false;
		// All keys are a single char keys
		_json_payload[0] = '{';
		_json_payload[1] = '"';
		_json_payload[3] = '"';
		_json_payload[4] = ':';
		char previous_key = _json_payload[2];
		size_t position_i = 0;
		size_t position_c = 0;
		// {"m":4,"b":1,"t":"green","a":"off","f":"esp","i":36843}
		for (size_t json_i = 6; json_i < _json_length; ++json_i) {	// 4 because it's the shortest position possible for ':'
			if (json_i + 4 < _json_length &&	// All keys are a single char keys
				_json_payload[json_i] == ',' && _json_payload[json_i + 1] == '"' && _json_payload[json_i + 3] == '"') {

				_json_payload[json_i + 4] = ':';

				if (_json_payload[json_i + 2] == 'i') {
					if (position_i) {
						if (position_c) {
							_json_payload[position_c] = 'X';	// Unknown, it can only be one 'c', the last key
						}
						position_c = json_i + 2;
						_json_payload[position_c] = 'c';	// Must be 'c', because it can only be one 'i'
						repeated_keys = true;
					} else {
						position_i = json_i + 2;
					}
				} else if (_json_payload[json_i + 2] == 'c') {	// 'c' comes after 'i'
					if (position_c) {
						if (position_i) {
							_json_payload[position_c] = 'X';	// Unknown, it can only be one 'c'
						} else {
							_json_payload[position_c] = 'i';	// The position_c must be 'i' because 'c' is the last one
						}
						repeated_keys = true;
					}
					position_c = json_i + 2;
				}
			}
			if (json_i + 1 < _json_length && _json_payload[json_i] == ':' &&
				(_json_payload[json_i - 1] == '"' || _json_payload[json_i + 1] == '"' ||	// ':' surrounded by at least one '"'
				!(_json_payload[json_i + 1] > '9' || _json_payload[json_i + 1] < '0'))) {	// ':' or with a number on the right side

				_json_payload[json_i - 4] = ',';
				_json_payload[json_i - 3] = '"';
				_json_payload[json_i - 1] = '"';
				if (_json_payload[json_i + 1] > '9' || _json_payload[json_i + 1] < '0' || _json_payload[json_i - 2] == 'f') {
					_json_payload[json_i + 1] = '"';
				}
				if (_json_payload[json_i - 5] > '9' || _json_payload[json_i - 5] < '0' || previous_key == 'f') {
					_json_payload[json_i - 5] = '"';
				}
				previous_key = _json_payload[json_i - 2];
			}
		}
		_json_payload[_json_length - 1] = '}';
		return repeated_keys;
	}


    /**
     * @brief Get targeting method
     * @return TalkerMatch enum indicating how message is targeted
     * @param name A char array of at least TALKIE_NAME_LEN
     * @param channel A pointer to a 8 bits number to get the channel number
     * 
     * Determines if message is for specific name, channel, broadcast, or invalid.
     */
	TalkerMatch _get_talker_match(char* name, uint8_t* channel) const {
		// To name
		if (get_to_name(name)) return TalkerMatch::TALKIE_MATCH_BY_NAME;
		// To channel
		uint32_t long_channel;
		if (get_key_value_number('t', &long_channel)) {
			*channel = static_cast<uint8_t>( long_channel );
			return TalkerMatch::TALKIE_MATCH_BY_CHANNEL;
		}
		// To everyone (Broadcasted)
		MessageValue message_value = get_message_value();
		if ((message_value > MessageValue::TALKIE_MSG_PING || has_nth_value_number(0)) && message_value != MessageValue::TALKIE_MSG_ERROR) {
			// Only TALK, CHANNEL and PING can be for ANY
			// AVOIDS DANGEROUS ALL AT ONCE TRIGGERING (USE CHANNEL INSTEAD)
			// AVOIDS DANGEROUS SETTING OF ALL CHANNELS AT ONCE
			return TalkerMatch::TALKIE_MATCH_FAIL;
		}
		return TalkerMatch::TALKIE_MATCH_ANY;
	}


    // ============================================
    // MESSAGE TARGETING
    // ============================================

    /**
     * @brief Compare with buffer content
     * @param buffer Buffer to compare with
     * @param length Length of buffer
     * @return true if content matches exactly
     */
	bool compare_buffer(const char* buffer, size_t length) const {
		if (length == _json_length) {
			for (size_t char_j = 0; char_j < length; ++char_j) {
				if (buffer[char_j] != _json_payload[char_j]) {
					return false;
				}
			}
			return true;
		}
		return false;
	}


    // ============================================
    // FIELD EXISTENCE CHECKS
    // ============================================

    /**
     * @brief Check if key exists
     * @param key Key to check
     * @return true if key exists in JSON
     */
	bool has_key(char key) const {
		return _get_colon_position(key) > 0;
	}


	/** @brief Check if checksum field exists */ 
	bool has_checksum() const {
		return _get_colon_position('c') > 0;
	}


	/** @brief Check if identity field exists */ 
	bool has_identity() const {
		return _get_colon_position('i') > 0;
	}


	/** @brief Check if broadcast value field exists */ 
	bool has_broadcast_value() const {
		return _get_colon_position('b') > 0;
	}


	/** @brief Check if 'from' field exists */
	bool has_from() const {
		return _get_colon_position('f') > 0;
	}


	/** @brief Check if 'from' field is a string (name) */
	bool has_from_name() const {
		return _get_value_type('f') == ValueType::TALKIE_VT_STRING;
	}


	/** @brief Check if 'to' field exists */
	bool has_to() const {
		return _get_colon_position('t') > 0;
	}


	/** @brief Check if 'to' field is a string (name) */
	bool has_to_name() const {
		return _get_value_type('t') == ValueType::TALKIE_VT_STRING;
	}


	/** @brief Check if 'to' field is a number (channel) */
	bool has_to_channel() const {
		return _get_value_type('t') == ValueType::TALKIE_VT_INTEGER;
	}


	/** @brief Check if action field exists */
	bool has_action() const {
		return _get_colon_position('a') > 0;
	}


	/** @brief Check if system field exists */
	bool has_system() const {
		return _get_colon_position('s') > 0;
	}


	/** @brief Check if error field exists */
	bool has_error() const {
		return _get_colon_position('e') > 0;
	}


    /**
     * @brief Check if nth value field exists (0-9)
     * @param nth Index 0-9
     * @return true if field exists
     */
	bool has_nth_value(uint8_t nth) const {
		if (nth < 10) {
			return _get_colon_position('0' + nth) > 0;
		}
		return false;
	}


    /**
     * @brief Check if nth value is a string
     * @param nth Index 0-9
     * @return true if field exists and is string
     */
	bool has_nth_value_string(uint8_t nth) const {
		if (nth < 10) {
			return _get_value_type('0' + nth) == ValueType::TALKIE_VT_STRING;
		}
		return false;
	}


    /**
     * @brief Check if nth value is a number
     * @param nth Index 0-9
     * @return true if field exists and is number
     */
	bool has_nth_value_number(uint8_t nth) const {
		if (nth < 10) {
			return _get_value_type('0' + nth) == ValueType::TALKIE_VT_INTEGER;
		}
		return false;
	}

    // ============================================
    // FIELD VALUE CHECKS
    // ============================================

    /**
     * @brief Check if 'from' field matches name
     * @param name Name to compare with
     * @return true if 'from' field exists and matches
     */
	bool is_from_name(const char* name) const {
		char from_name[TALKIE_NAME_LEN];
		if (_get_value_string('f', from_name, TALKIE_NAME_LEN)) {
			return strcmp(from_name, name) == 0;
		}
		return false;
	}


    /**
     * @brief Check if 'to' field matches name
     * @param name Name to compare with
     * @return true if 'to' field is string and matches
     */
	bool is_to_name(const char* name) const {
		char to_name[TALKIE_NAME_LEN];
		if (_get_value_string('t', to_name, TALKIE_NAME_LEN)) {
			return strcmp(to_name, name) == 0;
		}
		return false;
	}


    /**
     * @brief Check if 'to' field is to name
     * @return true if 'to' field is to a name
     */
	bool is_to_name() const {
		char to_name[TALKIE_NAME_LEN];
		return _get_value_string('t', to_name, TALKIE_NAME_LEN);
	}


    /**
     * @brief Check if 'to' field matches channel
     * @param channel Channel number (0-254)
     * @return true if 'to' field is number and matches
     */
	bool is_to_channel(uint8_t channel) const {
		uint32_t to_channel;
		if (_get_value_number('t', &to_channel)) {
			return channel == (uint8_t)to_channel;
		}
		return false;
	}


    /**
     * @brief Checks if 'to' field is a channel
     * @return true if 'to' field is a channel
     */
	bool is_to_channel() const {
		uint32_t to_channel;
		return _get_value_number('t', &to_channel);
	}

	
    /**
     * @brief Check if message is intended for this recipient
     * @param name Recipient name
     * @param channel Recipient channel
     * @return true if message targets this name/channel or is broadcast
     * 
     * Rules:
     * - If 't' field is string: match against name
     * - If 't' field is number: match against channel
     * - No 't' field: broadcast message (true for all)
     */
	bool is_for_me(const char* name, uint8_t channel) const {
		return is_to_name(name) || is_to_channel(channel);
	}


    /**
     * @brief Check if 'action' field matches the name
     * @param name Action name to compare with
     * @return true if 'action' field is string and matches
     */
	bool is_action_name(const char* name) const {
		char action_name[TALKIE_NAME_LEN];
		if (_get_value_string('a', action_name, TALKIE_NAME_LEN)) {
			return strcmp(action_name, name) == 0;
		}
		return false;
	}


    /**
     * @brief Check if 'action' field matches index
     * @param index Channel number (0-254)
     * @return true if 'action' field is number and matches
     */
	bool is_action_index(uint8_t index) const {
		size_t colon_position = _get_colon_position('a');
		return colon_position 
			&& _get_value_type('a', colon_position) == ValueType::TALKIE_VT_INTEGER
			&& _get_value_number('a', colon_position) == index;
	}


    /**
     * @brief Get if it's not to be replied
     * @return true if it's not to be replied with echo
     */
	bool is_no_reply() const {
		return _get_colon_position('n') > 0;
	}


    /**
     * @brief Checks if it's noise
     * @return true if the message is noise
     */
	bool is_noise() const {
		return get_message_value() == MessageValue::TALKIE_MSG_NOISE;
	}


    /**
     * @brief Get if it is a recovery message
     * @return true if a recovery message
     */
	bool is_recover_message() const {
		return _get_colon_position('M') > 0;
	}


    // ============================================
    // GETTERS - FIELD VALUES
    // ============================================

    /**
     * @brief Get the key value type
     * @param key A single char like 'm'
     * @return ValueType enum, or TALKIE_VT_VOID if invalid index
     */
	ValueType get_key_value_type(char key) {
		return _get_value_type(key);
	}


    /**
     * @brief Get nth string
     * @param key A single char like 'm'
     * @param value_string An array of at least TALKIE_MAX_LEN (64)
     * @param size The size of TALKIE_MAX_LEN or more
     * @return false if nth string non existent
	 * 
	 * @note Sets name as '\0' if returns false
     */
	bool get_key_value_string(char key, char* value_string, size_t size = TALKIE_MAX_LEN) const {
		return _get_value_string(key, value_string, size);
	}


    /**
     * @brief Get the key value number
     * @param key A single char like 'i'
     * @return Extracted number, or 0 if key not found or not a number
     */
	uint32_t get_key_value_number(char key) const {
		return _get_value_number(key);
	}


    /**
     * @brief Get the key value number
     * @param key A single char like 'i'
     * @param key_number Pointer to a 8 bits number to get the value number
     * @return false if no valid number was found
     * 
     * @note This method checks if the number is well terminated and bounded
     */
	bool get_key_value_number(char key, uint8_t* key_number) const {
		uint32_t json_number;
		if (_get_value_number(key, &json_number) && json_number <= 0xFF) {
			*key_number = (uint8_t)json_number;
			return true;
		}
		return false;
	}


    /**
     * @brief Get the key value number
     * @param key A single char like 'i'
     * @param key_number Pointer to a 16 bits number to get the value number
     * @return false if no valid number was found
     * 
     * @note This method checks if the number is well terminated and bounded
     */
	bool get_key_value_number(char key, uint16_t* key_number) const {
		uint32_t json_number;
		if (_get_value_number(key, &json_number) && json_number <= 0xFFFF) {
			*key_number = (uint16_t)json_number;
			return true;
		}
		return false;
	}


    /**
     * @brief Get the key value number
     * @param key A single char like 'i'
     * @param key_number Pointer to a 32 bits number to get the value number
     * @return false if no valid number was found
     * 
     * @note This method checks if the number is well terminated and bounded
     */
	bool get_key_value_number(char key, uint32_t* key_number) const {
		uint32_t json_number;
		if (_get_value_number(key, &json_number)) {
			*key_number = json_number;
			return true;
		}
		return false;
	}


    /**
     * @brief Get message type
     * @return MessageValue enum, or TALKIE_MSG_NOISE if invalid
     */
	MessageValue get_message_value() const {
		return static_cast<MessageValue>( _get_value_number('m') );
	}


    /**
     * @brief Get recovery message type
     * @return MessageValue enum, or TALKIE_MSG_NOISE if invalid
     */
	MessageValue get_recover_message_value() const {
		return static_cast<MessageValue>( _get_value_number('M') );
	}


    /**
     * @brief Get checksum number
     * @return Checksum 16-bit value (0-65535)
     */
	uint16_t get_checksum() {
		return static_cast<uint16_t>(_get_value_number('c'));
	}


    /**
     * @brief Get checksum number
     * @param checksum Pointer to a 16 bits number to get the checksum
     * @return false if no valid number was found
     * 
     * @note This method checks if the number is well terminated and bounded
     */
	bool get_checksum(uint16_t* checksum) const {
		uint32_t json_number;
		if (_get_value_number('c', &json_number) && json_number <= 0xFFFF) {
			*checksum = (uint16_t)json_number;
			return true;
		}
		return false;
	}


    /**
     * @brief Get identity number
     * @return Identity 16-bit value (0-65535)
     */
	uint16_t get_identity() {
		return static_cast<uint16_t>(_get_value_number('i'));
	}


    /**
     * @brief Get identity number
     * @param identity Pointer to a 16 bits number to get the identity
     * @return false if no valid number was found
     * 
     * @note This method checks if the number is well terminated and bounded
     */
	bool get_identity(uint16_t* identity) const {
		uint32_t json_number;
		if (_get_value_number('i', &json_number) && json_number <= 0xFFFF) {
			*identity = (uint16_t)json_number;
			return true;
		}
		return false;
	}


    /**
     * @brief Get timestamp (alias for identity)
     * @return Timestamp value in milliseconds (0-65535)
     */
	uint16_t get_timestamp() {
		return get_identity();
	}


    /**
     * @brief Get timestamp number (alias for identity)
     * @param timestamp Pointer to a 16 bits number to get the timestamp
     * @return false if no valid number was found
     */
	bool get_timestamp(uint16_t* timestamp) const {
		return get_identity(timestamp);
	}


    /**
     * @brief Get broadcast type
     * @return BroadcastValue enum, or TALKIE_BC_NONE if invalid
     */
	BroadcastValue get_broadcast_value() const {
		return static_cast<BroadcastValue>( _get_value_number('b') );
	}


    /**
     * @brief Get the broadcast_value
     * @param broadcast_value Pointer to a BroadcastValue variable
     * @return false if no valid BroadcastValue was found
     * 
     * @note This method checks if the number is well terminated and bounded
     */
	bool get_broadcast_value(BroadcastValue* broadcast_value) const {
		uint32_t json_number;
		uint32_t self_number = static_cast<uint32_t>( BroadcastValue::TALKIE_BC_SELF );
		if (_get_value_number('b', &json_number) && json_number <= self_number) {
			*broadcast_value = static_cast<BroadcastValue>(json_number);
			return true;
		}
		return false;
	}


    /**
     * @brief Get roger/acknowledgment type
     * @return RogerValue enum, or TALKIE_RGR_NIL if invalid
     */
	RogerValue get_roger_value() const {
		return static_cast<RogerValue>( _get_value_number('r') );
	}


    /**
     * @brief Get system information type
     * @return SystemValue enum, or TALKIE_SYS_UNDEFINED if invalid
     */
	SystemValue get_system_value() const {
		return static_cast<SystemValue>( _get_value_number('s') );
	}


    /**
     * @brief Get error type
     * @return ErrorValue enum, or TALKIE_ERR_UNDEFINED if invalid
     */
	ErrorValue get_error_value() const {
		return static_cast<ErrorValue>( _get_value_number('e') );
	}
	

    /**
     * @brief Get target type
     * @return ValueType of 't' field
     */
	ValueType get_to_type() const {
		return _get_value_type('t');
	}


    /**
     * @brief Get sender name
     * @param name An array of at least TALKIE_NAME_LEN
     * @param size The size of TALKIE_NAME_LEN or more
     * @return false if name is non existent or non conformant to TALKIE_NAME_LEN
	 * 
	 * @note Sets name as '\0' if returns false
     */
    bool get_from_name(char* name, size_t size = TALKIE_NAME_LEN) const {
        return _get_value_string('f', name, size > TALKIE_NAME_LEN ? TALKIE_NAME_LEN : size);
    }


    /**
     * @brief Get sender name
     * @param name An array of at least TALKIE_NAME_LEN
     * @param size The size of TALKIE_NAME_LEN or more
     * @return false if name is non existent or non conformant to TALKIE_NAME_LEN
	 * 
	 * @note Sets name as '\0' if returns false
     */
    bool get_to_name(char* name, size_t size = TALKIE_NAME_LEN) const {
        return _get_value_string('t', name, size > TALKIE_NAME_LEN ? TALKIE_NAME_LEN : size);
    }
	

    /**
     * @brief Get target channel
     * @return Channel number (0-255)
     */
	uint8_t get_to_channel() const {
		uint32_t channel;
		if (_get_value_number('t', &channel)) {
			if (channel < 255) {
				return (uint8_t)channel;
			}
		}
		return 255;	// Means, no chanel
	}

	
    /**
     * @brief Get nth value type
     * @param nth Index 0-9
     * @return ValueType enum, or TALKIE_VT_VOID if invalid index
     */
	ValueType get_nth_value_type(uint8_t nth) {
		if (nth < 10) {
			return _get_value_type('0' + nth);
		}
		return ValueType::TALKIE_VT_VOID;
	}


    /**
     * @brief Get nth string
     * @param nth Index 0-9
     * @param value_string An array of at least TALKIE_MAX_LEN (64)
     * @param size The size of TALKIE_MAX_LEN or more
     * @return false if nth string non existent
     */
	bool get_nth_value_string(uint8_t nth, char* value_string, size_t size = TALKIE_MAX_LEN) const {
        if (_get_value_string('0' + nth, value_string, size)) {
            return true;
        }
        return false;  // failed
	}


    /**
     * @brief Get nth value as number
     * @param nth Index 0-9
     * @return Numeric value, or 0 if not number/invalid
     */
	uint32_t get_nth_value_number(uint8_t nth) const {
		if (nth < 10) {
			return _get_value_number('0' + nth);
		}
		return 0;
	}


    /**
     * @brief Get nth value as number
     * @param nth Index 0-9
     * @param number Pointer to a 32 bits number to get
     * @return false if no valid number was found
     */
	bool get_nth_value_number(uint8_t nth, uint32_t* number) const {
		if (nth < 10) {
			return _get_value_number('0' + nth, number);
		}
		return false;
	}


    /**
     * @brief Get nth value as number
     * @param nth Index 0-9
     * @param number Pointer to a 16 bits number to get
     * @return false if no valid number was found
     */
	bool get_nth_value_number(uint8_t nth, uint16_t* number) const {
		if (nth < 10) {
			uint32_t json_number;
			if (_get_value_number('0' + nth, &json_number) && json_number <= 0xFFFF) {
				*number = (uint16_t)json_number;
				return true;
			}
		}
		return false;
	}

	
    /**
     * @brief Get nth value as number
     * @param nth Index 0-9
     * @param number Pointer to a 8 bits number to get
     * @return false if no valid number was found
     */
	bool get_nth_value_number(uint8_t nth, uint8_t* number) const {
		if (nth < 10) {
			uint32_t json_number;
			if (_get_value_number('0' + nth, &json_number) && json_number <= 0xFF) {
				*number = (uint8_t)json_number;
				return true;
			}
		}
		return false;
	}

	
    /**
     * @brief Get nth value as boolean
     * @param nth Index 0-9
     * @return Boolean value, or false if not number/invalid
     */
	bool get_nth_value_boolean(uint8_t nth) const {
		if (nth < 10) {
			return _get_value_number('0' + nth) != 0;
		}
		return false;
	}

	
    /**
     * @brief Get nth value as number
     * @param nth Index 0-9
     * @param bool_number Pointer to a bool to get
     * @return false if no valid boolean was found
     */
	bool get_nth_value_boolean(uint8_t nth, bool* boolean) const {
		if (nth < 10) {
			uint32_t json_number;
			if (_get_value_number('0' + nth, &json_number) && json_number <= 1) {
				*boolean = (bool)json_number;
				return true;
			}
		}
		return false;
	}


    /**
     * @brief Get action field type
     * @return ValueType of 'a' field
     */
	ValueType get_action_type() const {
		return _get_value_type('a');
	}


    /**
     * @brief Get action name
     * @param name An array of at least TALKIE_NAME_LEN
     * @param size The size of TALKIE_NAME_LEN or more
     * @return false if name is non existent or non conformant to TALKIE_NAME_LEN
	 * 
	 * @note Sets name as '\0' if returns false
     */
    bool get_action_name(char* name, size_t size = TALKIE_NAME_LEN) const {
        return _get_value_string('a', name, size > TALKIE_NAME_LEN ? TALKIE_NAME_LEN : size);
    }


    /**
     * @brief Get action as a number
     * @return The action index
     */
	uint8_t get_action_index() const {
		uint32_t index;
		if (_get_value_number('a', &index)) {
			if (index < 255) {
				return (uint8_t)index;
			}
		}
		return 255;	// Means, no index
	}


    // ============================================
    // REMOVERS - FIELD DELETION
    // ============================================

    /**
     * @brief Remove the key value pair
     * @param key A single char like 'i'
     * @return false if it wasn't able to remove or the field is non existent
     */
	void remove_key_field(char key) {
		_remove_field(key);
	}


    /** @brief Remove checksum field */
	void remove_checksum() {
		_remove_field('c');
	}


    /** @brief Remove message field */
	void remove_message() {
		_remove_field('m');
	}


    /** @brief Remove recover message field */
	void remove_recover_message() {
		_remove_field('M');
	}


	/** @brief Remove from field */
	void remove_from() {
		_remove_field('f');
	}


	/** @brief Remove to field */
	void remove_to() {
		_remove_field('t');
	}


	/** @brief Remove identity field */
	void remove_identity() {
		_remove_field('i');
	}


	/** @brief Remove timestamp field */
	void remove_timestamp() {
		_remove_field('i');
	}


	/** @brief Remove broadcast field */
	void remove_broadcast_value() {
		_remove_field('b');
	}


	/** @brief Remove action field */
	void remove_action() {
		_remove_field('a');
	}


	/** @brief Remove roger field */
	void remove_roger_value() {
		_remove_field('r');
	}


	/** @brief Remove system field */
	void remove_system_value() {
		_remove_field('s');
	}


    /**
     * @brief Remove nth value field
     * @param nth Index 0-9
     */
	void remove_nth_value(uint8_t nth) {
		if (nth < 10) _remove_field('0' + nth);
	}


    /**
     * @brief Remove all the nth values
     * @return true if removed at least one value
     */
	void remove_all_nth_values() {
		for (uint8_t nth = 0; nth < 10; ++nth) {
			remove_nth_value(nth);
		}
	}


	/** @brief Remove no reply field */
	void remove_no_reply() {
		_remove_field('n');
	}


    // ============================================
    // SETTERS - FIELD MODIFICATION
    // ============================================

	/**
     * @brief Set a number for a given key
     * @param key A single char like 'i'
     * @param number An 32-bit integer
     * @return true if successful
     */
	bool set_key_number(char key, uint32_t number) {
		return _set_value_number(key, number);
	}
	

    /**
     * @brief Set nth value as string
     * @param key A single char like 'i'
     * @param in_string String value pointer
     * @param size the size of the in_string buffer (not the length of the string) (TALKIE_MAX_LEN by default)
     * @return true if successful
	 * 
	 * @note a string can't be bigger than TALKIE_MAX_LEN
     */
	bool set_key_string(char key, const char* in_string, size_t size = TALKIE_MAX_LEN) {
		return _set_value_string(key, in_string, size);
	}


    /**
     * @brief Set message type
     * @param message_value Message type enum
     * @return true if field exists and was updated
     */
	bool set_message_value(MessageValue message_value) {
		return _set_value_single_digit_number('m', static_cast<uint32_t>(message_value));
	}


    /**
     * @brief Set recover message type
     * @param message_value Message type enum
     * @return true if field exists and was updated
     */
	bool set_recover_message_value(MessageValue message_value) {
		return _set_value_single_digit_number('M', static_cast<uint32_t>(message_value));
	}


	/**
     * @brief Set identity number
     * @param identity Identity value (0-65535)
     * @return true if successful
     */
	bool set_identity(uint16_t identity) {
		return _set_value_number('i', identity);
	}


    /**
     * @brief Set identity to current millis()
     * @return true if successful
     */
	bool set_identity() {
		uint16_t identity = (uint16_t)millis();
		return _set_value_number('i', identity);
	}


    /**
     * @brief Set timestamp (alias for identity)
     * @param timestamp Timestamp value in milliseconds
     * @return true if successful
     */
	bool set_timestamp(uint16_t timestamp) {
		return _set_value_number('i', timestamp);
	}


    /**
     * @brief Set timestamp to current millis()
     * @return true if successful
     */
	bool set_timestamp() {
		return set_identity();
	}


    /**
     * @brief Set sender name
     * @param name Sender name string
     * @return true if successful
     */
	bool set_from_name(const char* name) {
		return _set_value_string('f', name, TALKIE_NAME_LEN);
	}


    /**
     * @brief Set target name
     * @param name Target name string
     * @return true if successful
     */
	bool set_to_name(const char* name) {
		return _set_value_string('t', name, TALKIE_NAME_LEN);
	}


    /**
     * @brief Set target channel
     * @param channel Channel number (0-254)
     * @return true if successful
     */
	bool set_to_channel(uint8_t channel) {
		return _set_value_number('t', channel);
	}


    /**
     * @brief Set action name
     * @param name Action name string
     * @return true if successful
     */
	bool set_action_name(const char* name) {
		return _set_value_string('a', name, TALKIE_NAME_LEN);
	}


    /**
     * @brief Set action index
     * @param index Action index
     * @return true if successful
     */
	bool set_action_index(uint8_t index) {
		return _set_value_number('a', index);
	}


    /**
     * @brief Set broadcast type
     * @param broadcast_value Broadcast type enum
     * @return true if successful
     */
	bool set_broadcast_value(BroadcastValue broadcast_value) {
		return _set_value_single_digit_number('b', static_cast<uint32_t>(broadcast_value));
	}


    /**
     * @brief Set roger/acknowledgment type
     * @param roger_value Roger type enum
     * @return true if successful
     */
	bool set_roger_value(RogerValue roger_value) {
		return _set_value_single_digit_number('r', static_cast<uint32_t>(roger_value));
	}


    /**
     * @brief Set system information type
     * @param system_value System type enum
     * @return true if successful
     */
	bool set_system_value(SystemValue system_value) {
		return _set_value_single_digit_number('s', static_cast<uint32_t>(system_value));
	}


    /**
     * @brief Set error type
     * @param error_value Error type
     * @return true if successful
     */
	bool set_error_value(ErrorValue error_value) {
		return _set_value_single_digit_number('e', static_cast<uint32_t>(error_value));
	}


    /**
     * @brief Set nth value as number
     * @param nth Index 0-9
     * @param number Numeric value
     * @return true if successful
     */
	bool set_nth_value_number(uint8_t nth, uint32_t number) {
		if (nth < 10) {
			return _set_value_number('0' + nth, number);
		}
		return false;
	}


	// The Manifesto class description shouldn't be greater than 54 chars
	// {"m":7,"b":1,"i":58485,"f":"","t":"","0":"","c":11266} <-- 128 - (54 + 2*10) = 54

    /**
     * @brief Set nth value as string
     * @param nth Index 0-9
     * @param in_string String value pointer
     * @param size the size of the in_string buffer (not the length of the string) (TALKIE_MAX_LEN by default)
     * @return true if successful
	 * 
	 * @note a string can't be bigger than TALKIE_MAX_LEN
     */
	bool set_nth_value_string(uint8_t nth, const char* in_string, size_t size = TALKIE_MAX_LEN) {
		if (nth < 10) {
			return _set_value_string('0' + nth, in_string, size);
		}
		return false;
	}


    /**
     * @brief Set as a No Reply for `call` messages
	 * 
	 * There will be no echo for this message if it's a `call` one
	 * 
     * @return true if successful
     */
	bool set_no_reply() {
		return _set_value_number('n', 1);
	}


    /**
     * @brief Replaces a key with a different key
     * @param old_key Old key to be replaced
     * @param new_key New key to be inplace of the replaced key
     * @return true if it was able to replace the key
     */
	bool replace_key(char old_key, char new_key) {
		size_t key_position = _get_key_position(old_key);
		if (key_position) {
			_json_payload[key_position] = new_key;
			return true;
		}
		return false;
	}


    /**
     * @brief Swap 'to' with 'from' fields
     */
	void swap_to_with_from() {
		size_t key_to_position = _get_key_position('t');
		if (key_to_position) {
			size_t key_from_position = _get_key_position('f');
			_json_payload[key_to_position] = 'f';
			if (key_from_position) {
				_json_payload[key_from_position] = 't';
			}
		}
	}


    /**
     * @brief Swap 'from' with 'to' fields
     */
	void swap_from_with_to() {
		size_t key_from_position = _get_key_position('f');
		if (key_from_position) {
			size_t key_to_position = _get_key_position('t');
			_json_payload[key_from_position] = 't';
			if (key_to_position) {
				_json_payload[key_to_position] = 'f';
			}
		}
	}


    /**
     * @brief Swaps 'M' with 'm' fields, converting this way to a regular message
     */
	bool convert_recovery_message_to_message() {
		return replace_key('M', 'm');
	}

};


#endif // JSON_MESSAGE_HPP
