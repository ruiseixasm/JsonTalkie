/**
 * @file    M_GreenManifesto.hpp
 * @author  Rui Seixas Monteiro
 * @brief   A Manifesto targeted to an Arduino with a Green led on the pin defined by `GREEN_LED`.
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/manifestos
 * 
 * Actions:
 *  - on: Turns the Green led on
 *  - off: Turns the Green led off
 *  - state: Gets the state of the Green led
 *  - bpm_10: An example of getting and setting a variable in a single action
 *  - toggle: Toggles a remote Blue led on the Talker 'blue'
 * 
 * Hardware:
 * - Any type of Arduino compatible board will work.
 * 
 * Created: 2026-02-10
 */

#ifndef GREEN_MANIFESTO_HPP
#define GREEN_MANIFESTO_HPP

#include <TalkerManifesto.hpp>

// #define GREEN_TALKER_DEBUG


class M_GreenManifesto : public TalkerManifesto {
public:

	// The Manifesto class description shouldn't be greater than 42 chars
	// {"m":7,"f":"","s":1,"b":1,"t":"","i":58485,"0":"","1":1,"c":11266} <-- 128 - (66 + 2*10) = 42
    const char* class_description() const override { return "GreenManifesto"; }

    M_GreenManifesto() : TalkerManifesto() {}	// Constructor

protected:

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

	// The Action pair name and description shouldn't be greater than 40 chars
	// {"m":7,"b":1,"i":6442,"f":"","t":"","0":255,"1":"","2":"","c":25870} <-- 128 - (68 + 2*10) = 40

	// ------------- MAXIMUM SIZE RULER --------------|
	//	 "name", "123456789012345678901234567890123456"
    const Action actions[5] = {
		{"on", "Turns led ON"},
		{"off", "Turns led OFF"},
		{"state", "The actual state of the led"},
		{"bpm_10", "Gets/Sets the Tempo in BPM x 10"},
		{"toggle", "Toggles 'blue' led on and off"}
    };
    
    bool _is_led_on = false;  // keep track of state yourself, by default it's off
    uint16_t _bpm_10 = 1200;
	uint8_t _blue_led_on = 0;


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
				#ifdef GREEN_MANIFESTO_DEBUG
				Serial.println(F("\tCase 0 - Turning LED ON"));
				#endif
		
				if (!_is_led_on) {
				#ifdef GREEN_LED
					#ifdef GREEN_MANIFESTO_DEBUG
						Serial.print(F("\tGREEN_LED IS DEFINED as: "));
						Serial.println(GREEN_LED);
					#endif
					digitalWrite(GREEN_LED, HIGH);
				#else
					#ifdef GREEN_MANIFESTO_DEBUG
						Serial.println(F("\tGREEN_LED IS NOT DEFINED in this context!"));
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
				#ifdef GREEN_MANIFESTO_DEBUG
				Serial.println(F("\tCase 1 - Turning LED OFF"));
				#endif
		
				if (_is_led_on) {
				#ifdef GREEN_LED
					digitalWrite(GREEN_LED, LOW);
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
				if (json_message.has_nth_value_number(0)) {
					_bpm_10 = json_message.get_nth_value_number(0);
				} else {
					json_message.set_nth_value_number(0, _bpm_10);
				}
                return true;
            break;
				
            case 4:
			{
				JsonMessage toggle_blue_on_off(MessageValue::TALKIE_MSG_CALL, BroadcastValue::TALKIE_BC_LOCAL);
				toggle_blue_on_off.set_to_name("blue");
				if (_blue_led_on++ % 2) {
					toggle_blue_on_off.set_action_name("off");
				} else {
					toggle_blue_on_off.set_action_name("on");
				}
                return talker.transmitToRepeater(toggle_blue_on_off);
			}
            break;
			
			default: break;
		}
		return false;
	}
    
};


#endif // GREEN_MANIFESTO_HPP
