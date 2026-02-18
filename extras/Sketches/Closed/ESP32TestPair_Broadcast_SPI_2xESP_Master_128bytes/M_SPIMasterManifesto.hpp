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
#ifndef DUO_TESTER_MANIFESTO_HPP
#define DUO_TESTER_MANIFESTO_HPP

#include <TalkerManifesto.hpp>

// #define CYCLER_MANIFESTO_DEBUG


// LED_BUILTIN is already defined by ESP32 platform
// Typically GPIO2 for most ESP32 boards
#ifndef LED_BUILTIN
  #define LED_BUILTIN 2  // Fallback definition if not already defined
#endif


class M_SPIMasterManifesto : public TalkerManifesto {
public:

	// The Manifesto class description shouldn't be greater than 42 chars
	// {"m":7,"f":"","s":1,"b":1,"t":"","i":58485,"0":"","1":1,"c":11266} <-- 128 - (66 + 2*10) = 42
    const char* class_description() const override { return "SPIMasterManifesto"; }

    M_SPIMasterManifesto() : TalkerManifesto() {

		_toggle_yellow_on_off.set_to_name("slave");
	}	// Constructor

protected:

	uint16_t _self_blink_time = 0;

	uint32_t _last_blink = 0;
	uint8_t _yellow_led_on = 0;
	uint32_t _cyclic_period_ms = 60;
	bool _cyclic_transmission = true;	// true by default

	JsonMessage _toggle_yellow_on_off{
		MessageValue::TALKIE_MSG_CALL,
		BroadcastValue::TALKIE_BC_LOCAL
	};

    uint16_t _total_calls = 0;
    uint16_t _total_echoes = 0;

	uint8_t _burst_toggles = 0;
	uint32_t _burst_spacing_us = 0;
	uint32_t _last_burst_us = 0;

	// For ping
	char _original_talker[TALKIE_NAME_LEN];
	uint16_t _trace_message_timestamp;

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

	// The Action pair name and description shouldn't be greater than 40 chars
	// {"m":7,"b":1,"i":6442,"f":"","t":"","0":255,"1":"","2":"","c":25870} <-- 128 - (68 + 2*10) = 40

	// ------------- MAXIMUM SIZE RULER --------------|
	//	 "name", "123456789012345678901234567890123456"
    Action actions[8] = {
		{"period", "Sets cycle period milliseconds"},
		{"enable", "Enables 1sec cyclic transmission"},
		{"disable", "Disables 1sec cyclic transmission"},
		{"calls", "Gets total calls and their echoes"},
		{"reset", "Resets the totals counter"},
		{"burst", "Sends many messages at once"},
		{"spacing", "Burst spacing in microseconds"},
		{"ping", "Ping talkers by name or channel"}
    };
    
public:

    const Action* _getActionsArray() const override { return actions; }

    // Size methods
    uint8_t _actionsCount() const override { return sizeof(actions)/sizeof(Action); }


	void _loop(JsonTalker& talker) override {
		
		if ((uint16_t)millis() - _self_blink_time > 50) {	// turns off for 50 milliseconds
			
			digitalWrite(LED_BUILTIN, HIGH);
		}

		if (millis() - _last_blink > _cyclic_period_ms) {
			_last_blink = millis();

			if (_cyclic_transmission) {
				if (_yellow_led_on++ % 2) {
					_toggle_yellow_on_off.set_action_name("off");
				} else {
					_toggle_yellow_on_off.set_action_name("on");
				}
				talker.transmitToRepeater(_toggle_yellow_on_off);
				_total_calls++;
			}
		}

		if (_burst_toggles > 0 && micros() - _last_burst_us > _burst_spacing_us) {
			_burst_toggles--;
			
			if (_yellow_led_on++ % 2) {
				_toggle_yellow_on_off.set_action_name("off");
			} else {
				_toggle_yellow_on_off.set_action_name("on");
			}
			talker.transmitToRepeater(_toggle_yellow_on_off);
			_total_calls++;
			_last_burst_us = micros();
		}
	}

    
    // Index-based operations (simplified examples)
    bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
    	(void)talker_match;	// Silence unused parameter warning

		// Actual implementation would do something based on index
		switch(index) {

			case 0:
				if (json_message.has_nth_value_number(0)) {
					_cyclic_period_ms = json_message.get_nth_value_number(0);
				} else {
					json_message.set_nth_value_number(0, _cyclic_period_ms);
				}
				return true;
			break;
				
			case 1:
				_cyclic_transmission = true;
				return true;
			break;
				
			case 2:
				_cyclic_transmission = false;
				return true;
			break;
				
			case 3:
				json_message.set_nth_value_number(0, _total_calls);
				json_message.set_nth_value_number(1, _total_echoes);
				return true;
			break;
			
			case 4:
				_total_calls = 0;
				_total_echoes = 0;
				json_message.set_nth_value_number(0, _total_calls);
				json_message.set_nth_value_number(1, _total_echoes);
				return true;
			break;
			
			case 5:
			{
				_burst_toggles = 20;
				return true;
			}
			break;
			
			case 6:
				if (json_message.has_nth_value_number(0)) {
					_burst_spacing_us = json_message.get_nth_value_number(0);
				} else {
					json_message.set_nth_value_number(0, _burst_spacing_us);
				}
				return true;
			break;
				
			case 7:
			{
				// 1. Start by collecting info from message
				json_message.get_from_name(_original_talker);
				_trace_message_timestamp = json_message.get_identity();

				// 2. Repurpose it to be a LOCAL PING
				json_message.set_message_value(MessageValue::TALKIE_MSG_PING);
				json_message.remove_identity();
				if (json_message.get_nth_value_type(0) == ValueType::TALKIE_VT_STRING) {
					char value_name[TALKIE_NAME_LEN];
					if (json_message.get_nth_value_string(0, value_name, TALKIE_NAME_LEN)) {
						json_message.set_to_name(value_name);
					}
				} else if (json_message.get_nth_value_type(0) == ValueType::TALKIE_VT_INTEGER) {
					json_message.set_to_channel((uint8_t)json_message.get_nth_value_number(0));
				} else {	// Removes the original TO
					json_message.remove_to();	// Without TO works as broadcast
				}
				json_message.remove_nth_value(0);
				json_message.set_from_name(talker.get_name());	// Avoids the swapping

				// 3. Sends the message LOCALLY
				json_message.set_broadcast_value(BroadcastValue::TALKIE_BC_LOCAL);
				// No need to transmit the message, the normal ROGER reply does that for us!
				
				return true;
			}
			break;

			default: break;
		}
		return false;
	}


    void _echo(JsonTalker& talker, JsonMessage& json_message, MessageValue message_value, TalkerMatch talker_match) override {
		(void)talker_match;	// Silence unused parameter warning

		switch (message_value) {
			
			case MessageValue::TALKIE_MSG_CALL:
				digitalWrite(LED_BUILTIN, LOW);
				_self_blink_time = (uint16_t)millis();
				_total_echoes++;
			break;

			case MessageValue::TALKIE_MSG_PING:
			{
				// In condition to calculate the delay right away, no need to extra messages
				uint16_t actual_time = static_cast<uint16_t>(millis());
				uint16_t message_time = json_message.get_timestamp();	// must have
				uint16_t time_delay = actual_time - message_time;
				json_message.set_nth_value_number(0, time_delay);
				char from_name[TALKIE_NAME_LEN];
				json_message.get_from_name(from_name);
				json_message.set_nth_value_string(1, from_name);

				// Prepares headers for the original REMOTE sender
				json_message.set_to_name(_original_talker);
				json_message.set_from_name(talker.get_name());

				// Emulates the REMOTE original call
				json_message.set_identity(_trace_message_timestamp);

				// It's already an ECHO message, it's because of that that entered here
				// Finally answers to the REMOTE caller by repeating all other json fields
				json_message.set_broadcast_value(BroadcastValue::TALKIE_BC_REMOTE);
				talker.transmitToRepeater(json_message);
			}
			break;

			default: break;
		}
    }

};


#endif // DUO_TESTER_MANIFESTO_HPP
