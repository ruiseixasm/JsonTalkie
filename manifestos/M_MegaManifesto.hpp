/**
 * @file    M_MegaManifesto.hpp
 * @author  Rui Seixas Monteiro
 * @brief   This Manifesto concerns an Arduino Mega that manipulates its own builtin led and toggles
 * 			'on' and 'off' the led on the 'blue' Talker, that supposedly has the `M_BlueManifesto` and a Talker named 'blue'.
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/manifestos
 * 
 * Actions:
 *  - on: Turns the led on
 *  - off: Turns the led off
 *  - state: Gets the state of the led, 1 for 'on' and 0 for 'off'
 *  - toggle: Toggles 'on' and 'off' the remote 'blue' led
 *  - enable: Enables the cyclic 'talk' message to the remote 'buzzer' Talker
 *  - disable: Disables the cyclic 'talk' message to the remote 'buzzer' Talker
 * 
 * Hardware:
 * - Any type of Arduino compatible board will work.
 * 
 * Created: 2026-02-11
 */

#ifndef MEGA_MANIFESTO_HPP
#define MEGA_MANIFESTO_HPP

#include <TalkerManifesto.hpp>

// #define MEGA_MANIFESTO_DEBUG


class M_MegaManifesto : public TalkerManifesto {
public:

	// The Manifesto class description shouldn't be greater than 42 chars
	// {"m":7,"f":"","s":1,"b":1,"t":"","i":58485,"0":"","1":1,"c":11266} <-- 128 - (66 + 2*10) = 42
    const char* class_description() const override { return "MegaManifesto"; }

    M_MegaManifesto() : TalkerManifesto() {}	// Constructor


protected:

    Action calls[6] = {
		{"on", "Turns led ON"},
		{"off", "Turns led OFF"},
		{"state", "The actual state of the led"},
		{"toggle", "Toggles 'blue' led on and off"},
		{"enable", "Enables 1sec cyclic transmission"},
		{"disable", "Disables 1sec cyclic transmission"}
    };
    
    bool _is_led_on = false;  // keep track of state yourself, by default it's off
	uint8_t _blue_led_on = 0;
	uint32_t _last_talk = 0;
	bool _cyclic_transmission = false;

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
				#ifdef MEGA_MANIFESTO_DEBUG
				Serial.println(F("\tCase 0 - Turning LED ON"));
				#endif
		
				if (!_is_led_on) {
				#ifdef LED_BUILTIN
					#ifdef MEGA_MANIFESTO_DEBUG
						Serial.print(F("\tLED_BUILTIN IS DEFINED as: "));
						Serial.println(LED_BUILTIN);
					#endif
					digitalWrite(LED_BUILTIN, HIGH);
				#else
					#ifdef MEGA_MANIFESTO_DEBUG
						Serial.println(F("\tLED_BUILTIN IS NOT DEFINED in this context!"));
					#endif
				#endif
					_is_led_on = true;
					return true;
				} else {
					json_message.set_nth_value_string(0, "Already On!");
					return false;
				}
			}
			break;

			case 1:
			{
				#ifdef MEGA_MANIFESTO_DEBUG
				Serial.println(F("\tCase 1 - Turning LED OFF"));
				#endif
		
				if (_is_led_on) {
				#ifdef LED_BUILTIN
					digitalWrite(LED_BUILTIN, LOW);
				#endif
					_is_led_on = false;
				} else {
					json_message.set_nth_value_string(0, "Already Off!");
					return false;
				}
				return true;
			}
			break;
			
            case 2:
				json_message.set_nth_value_number(0, (uint32_t)_is_led_on);
                return true;
            break;
				
            case 3:
			{
				JsonMessage toggle_blue_on_off(MessageValue::TALKIE_MSG_CALL, BroadcastValue::TALKIE_BC_REMOTE);
				toggle_blue_on_off.set_to_name("blue");
				if (_blue_led_on++ % 2) {
					toggle_blue_on_off.set_action_name("off");
				} else {
					toggle_blue_on_off.set_action_name("on");
				}
                return talker.transmitToRepeater(toggle_blue_on_off);
			}
            break;
			
            case 4:
				_cyclic_transmission = true;
                return true;
            break;
				
            case 5:
				_cyclic_transmission = false;
                return true;
            break;
				
			default: break;
		}
		return false;
	}


	void _loop(JsonTalker& talker) override {
		
		if (millis() - _last_talk > 1000) {
			_last_talk = millis();

			JsonMessage buzzer_talk(MessageValue::TALKIE_MSG_TALK, BroadcastValue::TALKIE_BC_REMOTE);
			buzzer_talk.set_to_name("buzzer");
			if (_cyclic_transmission) talker.transmitToRepeater(buzzer_talk);
		}
	}
};


#endif // MEGA_MANIFESTO_HPP
