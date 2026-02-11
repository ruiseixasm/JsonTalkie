/**
 * @file    M_BlueManifesto.hpp
 * @author  Rui Seixas Monteiro
 * @brief   A Manifesto targeted to an Arduino or ESP32 board that controls its onboard blue led.
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/manifestos
 * 
 * Actions:
 *  - on: Turns the blue led on
 *  - off: Turns the blue led off
 *  - state: Returns the state of the led, o for off and 1 for on
 * 
 * Hardware:
 * - An ESP32 board.
 * 
 * Created: 2026-02-10
 */

#ifndef BLUE_MANIFESTO_HPP
#define BLUE_MANIFESTO_HPP

#include <TalkerManifesto.hpp>

#ifndef LED_BUILTIN	// For the case of an ESP32 board without the LED_BUILTIN defined
  #define LED_BUILTIN 2  // Fallback definition if not already defined
#endif

// #define BLUE_MANIFESTO_DEBUG


class M_BlueManifesto : public TalkerManifesto {
public:

	// The Manifesto class description shouldn't be greater than 42 chars
	// {"m":7,"f":"","s":1,"b":1,"t":"","i":58485,"0":"","1":1,"c":11266} <-- 128 - (66 + 2*10) = 42
    const char* class_description() const override { return "BlueManifesto"; }

    M_BlueManifesto() : TalkerManifesto()
	{
		pinMode(LED_BUILTIN, OUTPUT);
	}	// Constructor

    ~M_BlueManifesto()
	{	// ~TalkerManifesto() called automatically here
		digitalWrite(LED_BUILTIN, LOW);
		pinMode(LED_BUILTIN, INPUT);
	}	// Destructor


protected:

    bool _is_led_on = false;	// keep track of the led state, by default it's off

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

	// The Action pair name and description shouldn't be greater than 40 chars
	// {"m":7,"b":1,"i":6442,"f":"","t":"","0":255,"1":"","2":"","c":25870} <-- 128 - (68 + 2*10) = 40

	// ------------- MAXIMUM SIZE RULER --------------|
	//	 "name", "123456789012345678901234567890123456"
    Action calls[3] = {
		{"on", "Turns led ON"},
		{"off", "Turns led OFF"},
		{"state", "The actual state of the led"}
    };
    
public:
    
    const Action* _getActionsArray() const override { return calls; }
    uint8_t _actionsCount() const override { return sizeof(calls)/sizeof(Action); }


    // Index-based operations (simplified examples)
    bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
        (void)talker;		// Silence unused parameter warning
    	(void)talker_match;	// Silence unused parameter warning
		
		// Actual implementation would do something based on index
		switch(index) {

			case 0:
			{
				#ifdef BLUE_MANIFESTO_DEBUG
				Serial.println(F("\tCase 0 - Turning LED ON"));
				#endif
		
				if (!_is_led_on) {
					digitalWrite(LED_BUILTIN, HIGH);
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
				#ifdef BLUE_MANIFESTO_DEBUG
				Serial.println(F("\tCase 1 - Turning LED OFF"));
				#endif
		
				if (_is_led_on) {
				digitalWrite(LED_BUILTIN, LOW);
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
				
            default: return false;
		}
		return false;
	}
    
};


#endif // BLUE_MANIFESTO_HPP
