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
#ifndef ESP_MANIFESTO_HPP
#define ESP_MANIFESTO_HPP

#include <TalkerManifesto.hpp>

// #define ESP_MANIFESTO_DEBUG


class M_Esp66Manifesto : public TalkerManifesto {
public:

	// The Manifesto class description shouldn't be greater than 42 chars
	// {"m":7,"f":"","s":1,"b":1,"t":"","i":58485,"0":"","1":1,"c":11266} <-- 128 - (66 + 2*10) = 42
    const char* class_description() const override { return "Esp66Manifesto"; }

    M_Esp66Manifesto() : TalkerManifesto() {
	}	// Constructor


protected:

	bool _active_caller = false;
	// Avoids the initial triggering
	uint32_t _time_to_call = 60UL * 60 * 1000;
	uint32_t _time_to_live = 61UL * 60 * 1000;
    bool _is_led_on = false;	// keep track of the led state, by default it's off

	uint32_t _last_blink = 0;
	uint8_t _green_led_on = 0;
	bool _cyclic_transmission = false;

    Action calls[5] = {
		{"active", "Gets or sets the active status"},
		{"minutes", "Gets or sets the actual minutes"},
		{"state", "The actual state of the led"},
		{"enable", "Enables 1sec cyclic transmission"},
		{"disable", "Disables 1sec cyclic transmission"}
    };
    
public:
    
    const Action* _getActionsArray() const override { return calls; }

    // Size methods
    uint8_t _actionsCount() const override { return sizeof(calls)/sizeof(Action); }


    // Index-based operations (simplified examples)
    bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
        (void)talker;		// Silence unused parameter warning
        (void)talker_match;	// Silence unused parameter warning
		
		// Actual implementation would do something based on index
		switch(index) {

			case 0:
			{
				if (json_message.has_nth_value_number(0)) {
					if (json_message.get_nth_value_number(0)) {
						if (_active_caller) {
							json_message.set_nth_value_string(0, "Already active!");
						} else {
							_active_caller = true;
							return true;
						}
					} else {
						if (!_active_caller) {
							json_message.set_nth_value_string(0, "Already inactive!");
						} else {
							_active_caller = false;
							return true;
						}
					}
				} else {
					return json_message.set_nth_value_number(0, (uint32_t)_active_caller);
				}
			}
			break;

			case 1:
			{
				uint32_t present_time = millis();
				if (json_message.has_nth_value_number(0)) {
					uint32_t milliseconds_to_call = json_message.get_nth_value_number(0) % 60;
					milliseconds_to_call = (60UL - milliseconds_to_call) * 60 * 1000;
					_time_to_call = present_time + milliseconds_to_call;
					return true;
				} else {
					uint32_t minutes = (_time_to_call - present_time) / 1000 / 60;
					minutes = 59UL - minutes % 60;	// 0 based (0 to 59 minutes)
					return json_message.set_nth_value_number(0, minutes);
				}
			}
			break;
			
            case 2:
				json_message.set_nth_value_number(0, (uint32_t)_is_led_on);
                return true;
            break;
				
            case 3:
				_cyclic_transmission = true;
                return true;
            break;
				
            case 4:
				_cyclic_transmission = false;
                return true;
            break;
				
            default: break;
		}
		return false;
	}


	void _loop(JsonTalker& talker) override {
		uint32_t present_time = millis();
		if ((int32_t)(present_time - _time_to_call) >= 0) {
			if (_active_caller) {
				JsonMessage call_buzzer;
				call_buzzer.set_message_value(MessageValue::TALKIE_MSG_CALL);
				call_buzzer.set_broadcast_value(BroadcastValue::TALKIE_BC_REMOTE);
				call_buzzer.set_to_name("buzzer");
				call_buzzer.set_action_name("buzz");
				talker.transmitToRepeater(call_buzzer);
			}
			// The time needs to be updated regardless of the transmission above
			_time_to_call += 60UL * 60 * 1000;			// Add 60 minutes
		}
		if ((int32_t)(present_time - _time_to_live) >= 0) {
			digitalWrite(LED_BUILTIN, HIGH);	// In ESP8266 HIGH is LOW and LOW is HIGH
			_is_led_on = false;
			_time_to_live = _time_to_call + 1UL * 60 * 1000;	// Add 1 minute extra
		}

		if (millis() - _last_blink > 1000) {
			_last_blink = millis();

			JsonMessage toggle_green_on_off(MessageValue::TALKIE_MSG_CALL, BroadcastValue::TALKIE_BC_REMOTE);
			toggle_green_on_off.set_to_name("green");
			if (_green_led_on++ % 2) {
				toggle_green_on_off.set_action_name("off");
			} else {
				toggle_green_on_off.set_action_name("on");
			}
			if (_cyclic_transmission) talker.transmitToRepeater(toggle_green_on_off);
		}
	}


	void _echo(JsonTalker& talker, JsonMessage& json_message, MessageValue message_value, TalkerMatch talker_match) override {
        (void)talker;		// Silence unused parameter warning
        (void)json_message;	// Silence unused parameter warning
        (void)message_value;	// Silence unused parameter warning
        (void)talker_match;	// Silence unused parameter warning

		if (json_message.is_from_name("buzzer")) {
			_time_to_live = _time_to_call + 1UL * 60 * 1000;		// Add 1 minute extra
			digitalWrite(LED_BUILTIN, LOW);	// In ESP8266 HIGH is LOW and LOW is HIGH
			_is_led_on = true;
		}
    }

};


#endif // ESP_MANIFESTO_HPP
