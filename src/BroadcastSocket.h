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
 * @file BroadcastSocket.h
 * @brief Broadcast Socket interface for Talkie communication protocol
 * 
 * This class provides efficient, memory-safe input and output for the
 * JSONmessages which processing is started and finished by it.
 * 
 * @warning This class does not use dynamic memory allocation.
 *          All operations are performed on fixed-size buffers.
 * 
 * @author Rui Seixas Monteiro
 * @date Created: 2026-01-15
 * @version 2.0.0
 */


#ifndef BROADCAST_SOCKET_H
#define BROADCAST_SOCKET_H

#include <Arduino.h>    // Needed for Serial given that Arduino IDE only includes Serial in .ino files!
#include "JsonMessage.hpp"


// #define BROADCASTSOCKET_DEBUG
// #define BROADCASTSOCKET_DEBUG_NEW
// #define BROADCASTSOCKET_DEBUG_CHECKSUM
// #define BROADCASTSOCKET_DEBUG_CHECKSUM_FULL

// Readjust if necessary
#define MAX_NETWORK_PACKET_LIFETIME_MS 256UL    // 256 milliseconds

using LinkType			= TalkieCodes::LinkType;
using TalkerMatch 		= TalkieCodes::TalkerMatch;
using BroadcastValue 	= TalkieCodes::BroadcastValue;
using MessageValue 		= TalkieCodes::MessageValue;
using SystemValue 		= TalkieCodes::SystemValue;
using RogerValue 		= TalkieCodes::RogerValue;
using ErrorValue 		= TalkieCodes::ErrorValue;
using ValueType 		= TalkieCodes::ValueType;

class MessageRepeater;


/**
 * @class BroadcastSocket
 * @brief An Interface to be implemented as a Socket to receive and send `JsonMessage` content
 * 
 * The implementation of this class requires de definition of the methods, `_receive`,
 * `_send` and `class_description`. After receiving data, the method `_startTransmission`
 * shall be called.
 * 
 * @note Find `BroadcastSocket` implementation in https://github.com/ruiseixasm/JsonTalkie/tree/main/src/sockets.
 */
class BroadcastSocket {
protected:

	MessageRepeater* _message_repeater = nullptr;
	LinkType _link_type = LinkType::TALKIE_LT_NONE;
	bool _bridged = false;	///< Bridged: Can send and receive LOCAL broadcast messages too
    uint8_t _max_delay_ms = 5;
    bool _control_timing = false;
    unsigned long _last_local_time = 0;	// millis() compatible
    uint16_t _last_message_timestamp = 0;
    uint16_t _lost_count = 0;
    uint16_t _recoveries_count = 0;
    uint16_t _drops_count = 0;
    uint16_t _fails_count = 0;
	uint8_t _consecutive_errors = 0;	// Avoids a runaway flux of errors

	struct FromTalker {
		char name[TALKIE_NAME_LEN] = {'\0'};
		BroadcastValue broadcast = BroadcastValue::TALKIE_BC_NONE;
	};
	FromTalker _from_talker;
	

    enum CorruptionType : uint8_t {
		TALKIE_CT_CLEAN,
		TALKIE_CT_DATA,
		TALKIE_CT_CHECKSUM,
		TALKIE_CT_IDENTITY,
		TALKIE_CT_UNRECOVERABLE
    };

	struct CorruptedMessage {
		CorruptionType corruption_type;
		BroadcastValue broadcast;
		uint16_t identity;
		uint16_t checksum;
		uint16_t received_time;
		bool active = false;
	};
	CorruptedMessage _corrupted_message;

	
    // Constructor
    BroadcastSocket() {
		// Does nothing here
	}


    /**
     * @brief Sends the generated message by _startTransmission
	 *        to the Repeater
     */
	void _transmitToRepeater(JsonMessage& json_message);


    /** @brief Allows the visualization of the message before transmission */
	virtual void _showMessage(const JsonMessage& json_message) {
        (void)json_message;	// Silence unused parameter warning
	}


    /**
     * @brief Starts the transmission of the message received
     * @param json_message A json message to be transmitted to the repeater
     * @param check_integrity Enables or disables the message integrity checking,
	 *        usefull if you wan't to do it in the Socket implementation instead
     */
    void _startTransmission(JsonMessage& json_message, bool check_integrity = true) {
		
		#ifdef MESSAGE_DEBUG_TIMING
		Serial.print("\n\t");
		Serial.print(class_description());
		Serial.print(": ");
		#endif
			
		#ifdef BROADCASTSOCKET_DEBUG_NEW
		Serial.print(F("\t_startTransmission1.1: "));
		json_message.write_to(Serial);
		Serial.print(" | ");
		Serial.println(json_message._get_length());
		#endif
		
		if (check_integrity) {	// Validate message integrity

			#if defined(BROADCASTSOCKET_DEBUG_CHECKSUM)
			json_message._corrupt_payload(false);
			#elif defined(BROADCASTSOCKET_DEBUG_CHECKSUM_FULL)
			json_message._corrupt_payload(true);
			#endif

			size_t received_length = json_message._get_length();
			if (!json_message._validate_json()) {
				// Resets its initial length in order to be processed next, as error (checksum fail)
				json_message._set_length(received_length);
			}

			CorruptionType corruption_type = TALKIE_CT_CLEAN;
			uint16_t message_identity;

			if (!json_message.get_checksum(&_corrupted_message.checksum)) {
				
				if (!json_message.get_identity(&message_identity)) {
					corruption_type = TALKIE_CT_UNRECOVERABLE;
				} else {
					corruption_type = TALKIE_CT_CHECKSUM;
				}

			} else {
				json_message.remove_checksum();
				if (json_message._generate_checksum() != _corrupted_message.checksum) {
					
					if (!json_message.get_identity(&message_identity)) {
						corruption_type = TALKIE_CT_IDENTITY;
					} else {
						corruption_type = TALKIE_CT_DATA;
					}
				}
			}
			

			if (corruption_type != TALKIE_CT_CLEAN) {

				if (_consecutive_errors < MAXIMUM_CONSECUTIVE_ERRORS) {	// Avoids a runaway flux of errors

					JsonMessage error_message;
					error_message.set_message_value(MessageValue::TALKIE_MSG_ERROR);
					error_message.set_error_value(ErrorValue::TALKIE_ERR_CHECKSUM);

					if (!json_message.get_broadcast_value(&_corrupted_message.broadcast)) {
						_corrupted_message.broadcast = BroadcastValue::TALKIE_BC_NONE;
					} else {
						error_message.set_broadcast_value(_corrupted_message.broadcast);
					}

					++_lost_count;	// Non recoverable so far (+1)

					switch (corruption_type) 
					{
						case TALKIE_CT_DATA:
						case TALKIE_CT_CHECKSUM:
							error_message.set_identity(message_identity);
						break;
						
						case TALKIE_CT_UNRECOVERABLE:
							_corrupted_message.active = false;
							return;
						break;
						
						default: break;
					}

					if (_corrupted_message.broadcast == BroadcastValue::TALKIE_BC_NONE) {

						error_message.set_broadcast_value(BroadcastValue::TALKIE_BC_LOCAL);
						_finishTransmission(error_message);
						error_message.set_broadcast_value(BroadcastValue::TALKIE_BC_REMOTE);
						_finishTransmission(error_message);

					} else {
						_finishTransmission(error_message);
					}

					_corrupted_message.corruption_type = corruption_type;
					_corrupted_message.identity = message_identity;
					_corrupted_message.received_time = (uint16_t)millis();
					_corrupted_message.active = true;
					++_consecutive_errors;	// Avoids a runaway flux of errors
					
					#if defined(BROADCASTSOCKET_DEBUG_CHECKSUM) || defined(BROADCASTSOCKET_DEBUG_CHECKSUM_FULL)
					Serial.print(F("\t_startTransmission1.2: "));
					json_message.write_to(Serial);
					Serial.print(" | ");
					Serial.print(_corrupted_message.checksum);
					Serial.print(" | ");
					Serial.print(_corrupted_message.identity);
					Serial.print(" | ");
					Serial.println((int)_corrupted_message.corruption_type);
					Serial.print(F("\t_startTransmission1.3: "));
					error_message.write_to(Serial);
					Serial.print(" | ");
					Serial.println(error_message._get_length());
					#endif
			
				} else {
					++_lost_count;			// Non recoverable (+1)
				}
				return;
			}
		}

		_consecutive_errors = 0;	// Avoids a runaway flux of errors

		// At this point the message has its integrity guaranteed
		if (json_message.has_key('M')) {	// It's a Recovery message
			
			if (_corrupted_message.active) {
				if (json_message.replace_key('M', 'm')) {	// Removes the tag in order to be processed
		
					uint16_t message_checksum = json_message._generate_checksum();
					uint16_t message_identity = json_message.get_identity();
					
					#if defined(BROADCASTSOCKET_DEBUG_CHECKSUM) || defined(BROADCASTSOCKET_DEBUG_CHECKSUM_FULL)
					Serial.print(F("\t_startTransmission1.4: "));
					json_message.write_to(Serial);
					Serial.print(" | ");
					Serial.print(message_checksum);
					Serial.print(" | ");
					Serial.print(message_identity);
					Serial.print(" | ");
					Serial.println((int)_corrupted_message.corruption_type);
					#endif
		
					switch (_corrupted_message.corruption_type) 
					{
						case TALKIE_CT_DATA:
							if (message_identity == _corrupted_message.identity && message_checksum == _corrupted_message.checksum) {
								++_recoveries_count;	// It is a recovered message (+1)
								--_lost_count;			// Non recoverable (-1)
								_corrupted_message.active = false;
							} else {
								// Not for this Socket, let the Repeater send to other Sockets
								json_message.replace_key('m', 'M');	// Replaces the tag for other Socket
							}
						break;

						case TALKIE_CT_CHECKSUM:
							if (message_identity == _corrupted_message.identity) {
								++_recoveries_count;	// It is a recovered message (+1)
								--_lost_count;			// Non recoverable (-1)
								_corrupted_message.active = false;
							} else {
								// Not for this Socket, let the Repeater send to other Sockets
								json_message.replace_key('m', 'M');	// Replaces the tag for other Socket
							}
						break;

						case TALKIE_CT_IDENTITY:
							if (message_checksum == _corrupted_message.checksum) {
								++_recoveries_count;	// It is a recovered message (+1)
								--_lost_count;			// Non recoverable (-1)
								_corrupted_message.active = false;
							} else {
								// Not for this Socket, let the Repeater send to other Sockets
								json_message.replace_key('m', 'M');	// Replaces the tag for other Socket
							}
						break;
						
						default: break;
					}

				} else {
					return;	// Malformatted 'M'
				}
			}
		}


		if (json_message.has_broadcast_value()) {	// Mandatory field
			if (json_message.has_from()) {
				// From a Talker
				if (!(
					json_message.get_from_name(_from_talker.name) &&
					json_message.get_broadcast_value(&_from_talker.broadcast)
				)) {
					// Makes sure corrupt data isn't used
					_from_talker.name[0] = '\0';
					_from_talker.broadcast = BroadcastValue::TALKIE_BC_NONE;
					return;	// If fields exist they must be valid
				}
			// From a Socket
			} else if (json_message.is_noise()) {	// Reset name keeping
				// Resets the from talker data
				_from_talker.name[0] = '\0';
				_from_talker.broadcast = BroadcastValue::TALKIE_BC_NONE;
				return;	// It came from a Socket, no need to lose more time
			}
		}

		_showMessage(json_message);

		#ifdef BROADCASTSOCKET_DEBUG_NEW
		Serial.print(F("\t_startTransmission2: "));
		json_message.write_to(Serial);
		Serial.print(" | ");
		Serial.println(json_message._get_length());
		#endif
		
		if (_max_delay_ms > 0) {

			MessageValue message_code = json_message.get_message_value();
			if (message_code == MessageValue::TALKIE_MSG_CALL) {

				uint16_t message_timestamp = json_message.get_timestamp();

				#ifdef BROADCASTSOCKET_DEBUG
				Serial.print(F("_startTransmission2: Message code requires delay check: "));
				Serial.println((int)message_code);
				#endif

				#ifdef BROADCASTSOCKET_DEBUG
				Serial.print(F("_startTransmission3: Remote time: "));
				Serial.println(message_timestamp);
				#endif
			
				const unsigned long local_time = millis();
				
				if (_control_timing) {
					
					const uint16_t remote_delay = _last_message_timestamp - message_timestamp;  // Package received after

					if (remote_delay > 0 && remote_delay < MAX_NETWORK_PACKET_LIFETIME_MS) {    // Out of order package
						const uint16_t allowed_delay = static_cast<uint16_t>(_max_delay_ms);
						const uint16_t local_delay = local_time - _last_local_time;
						#ifdef BROADCASTSOCKET_DEBUG
						Serial.print(F("_startTransmission4: Local delay: "));
						Serial.println(local_delay);
						#endif
						if (remote_delay > allowed_delay || local_delay > allowed_delay) {
							#ifdef BROADCASTSOCKET_DEBUG
							Serial.print(F("_startTransmission5: Out of time package (remote delay): "));
							Serial.println(remote_delay);
							#endif
							
							if (_from_talker.broadcast != BroadcastValue::TALKIE_BC_NONE) {	// a valid from_talker name (set above)
								JsonMessage error_message(MessageValue::TALKIE_MSG_ERROR, _from_talker.broadcast);
								error_message.set_to_name(_from_talker.name);
								error_message.set_identity(json_message.get_identity());	// Already validated with checksum
								error_message.set_error_value(ErrorValue::TALKIE_ERR_DELAY);
								// Error messages can be anonymous messages without "from_name"
								_finishTransmission(error_message);
							}
							++_drops_count;
							return;
						}
					}
				}
				_last_local_time = local_time;
				_last_message_timestamp = message_timestamp;
				_control_timing = true;
			}
		}

		#ifdef MESSAGE_DEBUG_TIMING
		Serial.print(millis() - json_message._reference_time);
		#endif
		
		_transmitToRepeater(json_message);
		
		#ifdef MESSAGE_DEBUG_TIMING
		Serial.print(" | ");
		Serial.print(millis() - json_message._reference_time);
		#endif
		
    }

	
    /**
     * @brief Pure abstract method that creates a new `JsonMessage` based on the
	 *        receiving data by the socket
	 * 
     * @note This method shall call the method `_startTransmission` with the new created
	 *       `JsonMessage`.
     */
    virtual void _receive() = 0;


	/**
     * @brief Pure abstract method that sends via socket any received json message
     * @param json_message A json message able to be accessed by the subclass socket
	 * 
     * @note This method marks the end of the message cycle with `_finishTransmission`.
     */
    virtual bool _send(const JsonMessage& json_message) = 0;


public:
    // Delete copy/move operations
    BroadcastSocket(const BroadcastSocket&) = delete;
    BroadcastSocket& operator=(const BroadcastSocket&) = delete;
    BroadcastSocket(BroadcastSocket&&) = delete;
    BroadcastSocket& operator=(BroadcastSocket&&) = delete;

	
	/** @brief A getter for the class name to be returned for the `system` command */
    virtual const char* class_description() const = 0;

	
	/**
     * @brief Method intended to be called by the Repeater class by its public loop method
	 * 
     * @note This method being underscored means to be called internally only.
     */
    virtual void _loop() {
        // In theory, a UDP packet on a local area network (LAN) could survive
        // for about 4.25 minutes (255 seconds).
        // BUT in practice it won't more that 256 milliseconds given that is a Ethernet LAN
        if (_control_timing && millis() - _last_local_time > MAX_NETWORK_PACKET_LIFETIME_MS) {
            _control_timing = false;
        }
		if (_corrupted_message.active && (uint16_t)millis() - _corrupted_message.received_time > TALKIE_RECOVERY_TTL) {
			_corrupted_message.active = false;
		}
        _receive();
    }


    // ============================================
    // GETTERS - FIELD VALUES
    // ============================================
	
    /**
     * @brief Get the Link Type with the Message Repeater
     * @return Returns the Link Type (ex. UP_LINKED)
	 * 
     * @note Usefull if intended to be bridged (ex. UP_BRIDGED),
	 *       where the `LOCAL` messages are also broadcasted
     */
	LinkType getLinkType() const { return _link_type; }


    /**
     * @brief Get the maximum amount of delay a message can have before being dropped
     * @return Returns the delay in microseconds
     * 
     * @note A max delay of `0` means no message will be dropped,
	 *       this only applies to `CALL` messages value
     */
    uint8_t get_max_delay() const { return _max_delay_ms; }


    /**
     * @brief Get the total amount of lost messages in transmission
     * @return Returns the number of failed transmissions due to not having a valid message identity
     * 
     * @note A lost message is a received message that is corrupted and unrecoverable by missing an id
     */
    uint16_t get_lost_count() const { return _lost_count; }

	
    /**
     * @brief Get the total amount of recoveries in transmission
     * @return Returns the number of failed transmissions, wrong checksum
     * 
     * @note A missed message is a received message that is corrupted but still has an id (recoverable)
     */
    uint16_t get_recoveries_count() const { return _recoveries_count; }

	
    /**
     * @brief Get the total amount of call messages already dropped
     * @return Returns the number of dropped call messages
     * 
     * @note A dropped message is a received message that was received after the acceptable `max_delay`
     */
    uint16_t get_drops_count() const { return _drops_count; }

	
    /**
     * @brief Get the total amount of sending messages fails
     * @return Returns the number of failures in sending messages
     * 
     * @note A failed message is a message that failed to be sent
     */
    uint16_t get_fails_count() const { return _fails_count; }

	
    /**
     * @brief Get the the bridged configuration of the Socket
     * @return true if bridged and false if unbridged
     */
	bool isBridged() const {
		return _bridged;
	}
	

    // ============================================
    // SETTERS - FIELD MODIFICATION
    // ============================================

    /**
     * @brief Intended to be used by the Message Repeater only
     * @param message_repeater The Message Repeater pointer
     * @param link_type The Link Type with the Message Repeater
     * 
     * @note This method is used by the Message Repeater to set up the Socket
     */
	virtual void _setLink(MessageRepeater* message_repeater, LinkType link_type);


    /**
     * @brief Sets the maximum amount of delay a message can have before being dropped
     * @param max_delay_ms The maximum amount of delay in milliseconds
     * 
     * @note A max delay of `0` means no message will be dropped,
	 *       this only applies to `CALL` messages value
     */
    void set_max_delay(uint8_t max_delay_ms = 5) { _max_delay_ms = max_delay_ms; }
	

    /**
     * @brief Sets the Socket as Bridged
     * 
     * @note As bridged, the Socket receives Local messages even if uplinked
     */
	void bridgeSocket() { 
        _bridged = true;
    }
    

    /**
     * @brief Sets the Socket as unbridged (removes the bridge)
     * 
     * @note As bridged, the Socket receives Local messages even if uplinked
     */
    void unbridgeSocket() { 
        _bridged = false; 
    }


	/**
     * @brief The final step in a cycle of processing a json message in which the
	 *        json message content is sent accordingly to the `_send` method implementation
     * @param json_message A json message which buffer is to be sent
	 * 
     * @note This method marks the end of the message transmission cycle.
     */
    bool _finishTransmission(JsonMessage& json_message) {

		bool message_sent = false;

		#ifdef BROADCASTSOCKET_DEBUG_NEW
		Serial.print(F("socketSend1: "));
		json_message.write_to(Serial);
		Serial.println();  // optional: just to add a newline after the JSON
		#endif

		#ifdef MESSAGE_DEBUG_TIMING
		Serial.print(" | ");
		Serial.print(millis() - json_message._reference_time);
		#endif
			
		if (json_message._get_length() && json_message._insert_checksum()) {
			
			#ifdef BROADCASTSOCKET_DEBUG_NEW
			Serial.print(F("\tsocketSend2: "));
			json_message.write_to(Serial);
			Serial.println();  // optional: just to add a newline after the JSON
			#endif

			message_sent = _send(json_message);

			#ifdef MESSAGE_DEBUG_TIMING
			Serial.print(" | ");
			Serial.print(millis() - json_message._reference_time);
			#endif

			if (!message_sent) {
				++_fails_count;
			}
		}
		return message_sent;
    }

};

#endif // BROADCAST_SOCKET_H
