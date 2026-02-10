/**
 * @file    M_BlackManifesto.hpp
 * @author  Rui Seixas Monteiro
 * @brief   A Manifesto targeted to a Black Box that has an Arduino Nano connected to a Buzzer on pin 3.
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/manifestos
 * 
 * Actions:
 *  - buzz: Triggers the buzzer
 *  - ms: Sets the duration in milliseconds of the buzzing
 * 
 * Hardware:
 * - An Arduino Nano and a buzzer.
 * 
 * Created: 2026-02-10
 */

#ifndef BLACK_MANIFESTO_HPP
#define BLACK_MANIFESTO_HPP

#include <TalkerManifesto.hpp>

// #define BLACK_MANIFESTO_DEBUG

#define BUZZ_PIN 3	// External BLACK BOX pin


class M_BlackManifesto : public TalkerManifesto {
public:

	// The Manifesto class description shouldn't be greater than 42 chars
	// {"m":7,"f":"","s":1,"b":1,"t":"","i":58485,"0":"","1":1,"c":11266} <-- 128 - (66 + 2*10) = 42
    const char* class_description() const override { return "BlackManifesto"; }

    M_BlackManifesto() : TalkerManifesto() {}	// Constructor


protected:

    uint16_t _buzz_duration_ms = 100;
	uint16_t _buzz_start = 0;

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

    Action calls[2] = {
		{"buzz", "Buzz for a while"},
		{"ms", "Gets and sets the buzzing duration"}
    };
    
public:

    const Action* _getActionsArray() const override { return calls; }

    // Size methods
    uint8_t _actionsCount() const override { return sizeof(calls)/sizeof(Action); }


	void _loop(JsonTalker& talker) override {
        (void)talker;		// Silence unused parameter warning
		if ((uint16_t)millis() - _buzz_start > _buzz_duration_ms) {
			#ifdef BUZZ_PIN
			digitalWrite(BUZZ_PIN, LOW);
			#endif
		}
	}

    
    // Index-based operations (simplified examples)
    bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
        (void)talker;		// Silence unused parameter warning
    	(void)talker_match;	// Silence unused parameter warning

		// Actual implementation would do something based on index
		switch(index) {

			case 0:
			{
				#ifdef BLACK_MANIFESTO_DEBUG
				Serial.println(F("\tCase 0 - Triggering the buzzer"));
				#endif
		
				#ifdef BUZZ_PIN
				#ifdef BLACK_MANIFESTO_DEBUG
					Serial.print(F("\tBUZZ_PIN IS DEFINED as: "));
					Serial.println(BUZZ_PIN);
				#endif

				digitalWrite(BUZZ_PIN, HIGH);
				_buzz_start = (uint16_t)millis();

				#else
				#ifdef BLACK_MANIFESTO_DEBUG
					Serial.println(F("\tBUZZ_PIN IS NOT DEFINED in this context!"));
				#endif
				#endif

				return true;
			}
			break;

			case 1:
				if (json_message.has_nth_value_number(0)) {
					_buzz_duration_ms = (uint16_t)json_message.get_nth_value_number(0);
				} else {
					json_message.set_nth_value_number(0, _buzz_duration_ms);
				}
				return true;
			break;

			default: break;
		}
		return false;
	}
    

    void _echo(JsonTalker& talker, JsonMessage& json_message, MessageValue message_value, TalkerMatch talker_match) override {
		(void)talker;		// Silence unused parameter warning
        (void)message_value;	// Silence unused parameter warning
		(void)talker_match;	// Silence unused parameter warning

		char temp_string[TALKIE_MAX_LEN];
		json_message.get_from_name(temp_string);
		Serial.print( temp_string );
        Serial.print(" - ");
		
		ValueType value_type = json_message.get_nth_value_type(0);
		switch (value_type) {

			case ValueType::TALKIE_VT_STRING:
				json_message.get_nth_value_string(0, temp_string);
				Serial.println(temp_string);
			break;
			
			case ValueType::TALKIE_VT_INTEGER:
				Serial.println(json_message.get_nth_value_number(0));
			break;
			
			default:
            	Serial.println(F("Empty echo received!"));
			break;
		}
    }


    void _error(JsonTalker& talker, JsonMessage& json_message, ErrorValue error_value, TalkerMatch talker_match) override {
		(void)talker;		// Silence unused parameter warning
		(void)error_value;	// Silence unused parameter warning
		(void)talker_match;	// Silence unused parameter warning

		char temp_string[TALKIE_MAX_LEN];
		json_message.get_from_name(temp_string);
		Serial.print( temp_string );
        Serial.print(" - ");
		
		ValueType value_type = json_message.get_nth_value_type(0);
		switch (value_type) {

			case ValueType::TALKIE_VT_STRING:
				json_message.get_nth_value_string(0, temp_string);
				Serial.println(temp_string);
			break;
			
			case ValueType::TALKIE_VT_INTEGER:
				Serial.println(json_message.get_nth_value_number(0));
			break;
			
			default:
            	Serial.println(F("Empty echo received!"));
			break;
		}
    }

};


#endif // BLACK_MANIFESTO_HPP
