/**
 * @file    M_CallerManifesto.hpp
 * @author  Rui Seixas Monteiro
 * @brief   A Manifesto targeted to an Arduino with a `M_BlackManifesto` manifesto, this triggers the buzzer
 * 			in the targeted board for each 60 minutes, where the present minute can be given in order to sync
 * 			it with the real time.
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/manifestos
 * 
 * Actions:
 *  - active: Activates the triggering by giving the value 1 or disables it by given the value 0
 *  - minutes: Sets the actual hourly minute
 *  - state: Gets the state of the led meaning that the triggering of the buzzer was replied with an echo
 * 
 * Hardware:
 * - Any type of Arduino compatible board will work.
 * 
 * Created: 2026-02-10
 */

#ifndef CALLER_MANIFESTO_HPP
#define CALLER_MANIFESTO_HPP

#include <TalkerManifesto.hpp>

// #define CALLER_MANIFESTO_DEBUG


class M_CallerManifesto : public TalkerManifesto {
public:

	// The Manifesto class description shouldn't be greater than 42 chars
	// {"m":7,"s":1,"b":1,"f":"","t":"","i":58485,"0":"","1":1,"c":11266} <-- 128 - (66 + 2*10) = 42
    const char* class_description() const override { return "CallerManifesto"; }

    M_CallerManifesto() : TalkerManifesto() {
		pinMode(LED_BUILTIN, OUTPUT);
		digitalWrite(LED_BUILTIN, LOW); // Start with LED off
	}	// Constructor


protected:

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

	// The Action pair name and description shouldn't be greater than 40 chars
	// {"m":7,"b":1,"i":6442,"f":"","t":"","0":255,"1":"","2":"","c":25870} <-- 128 - (68 + 2*10) = 40

	// ------------- MAXIMUM SIZE RULER --------------|
	//	 "name", "123456789012345678901234567890123456"
    Action actions[3] = {
		{"active", "Gets or sets the active status"},
		{"minutes", "Gets or sets the actual minutes"},
		{"state", "The actual state of the led"}
    };
    
	bool _active_caller = false;
	uint32_t _time_to_call = 0;
	uint32_t _time_to_live = 0;
    bool _is_led_on = false;	// keep track of the led state, by default it's off

public:
    
    const Action* _getActionsArray() const override { return actions; }

    // Size methods
    uint8_t _actionsCount() const override { return sizeof(actions)/sizeof(Action); }


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
				
            default: break;
		}
		return false;
	}


	void _loop(JsonTalker& talker) override {
		uint32_t present_time = millis();
		// 32 bits is 0xFFFFFFFF (4 bytes)
		if ((int32_t)(present_time - _time_to_call) >= 0) {
			if (_active_caller) {
				JsonMessage call_buzzer;
				call_buzzer.set_message_value(MessageValue::TALKIE_MSG_CALL);
				call_buzzer.set_broadcast_value(BroadcastValue::TALKIE_BC_REMOTE);
				call_buzzer.set_to_name("nano");
				call_buzzer.set_action_name("buzz");
				talker.transmitToRepeater(call_buzzer);
			}
			// The time needs to be updated regardless of the transmission above
			_time_to_call += 60UL * 60 * 1000;			// Add 60 minutes
		}
		if ((int32_t)(present_time - _time_to_live) >= 0) {
			digitalWrite(LED_BUILTIN, LOW);
			_is_led_on = false;
			_time_to_live = _time_to_call + 1UL * 60 * 1000;	// Add 1 minute extra
		}
	}


	void _echo(JsonTalker& talker, JsonMessage& json_message, MessageValue message_value, TalkerMatch talker_match) override {
        (void)talker;		// Silence unused parameter warning
        (void)json_message;	// Silence unused parameter warning
        (void)message_value;	// Silence unused parameter warning
        (void)talker_match;	// Silence unused parameter warning

		if (json_message.is_from_name("nano")) {
			_time_to_live = _time_to_call + 1UL * 60 * 1000;		// Add 1 minute extra
			digitalWrite(LED_BUILTIN, HIGH);
			_is_led_on = true;
		}
    }
    
};


#endif // CALLER_MANIFESTO_HPP
