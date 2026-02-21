# JsonTalkie - Talker Manifestos

Here you can find many Manifestos that are examples of `TalkerManifesto` interface implementation.

To include any of these Manifestos, just add the following include line accordingly:
```cpp
#include "M_BlueManifesto.hpp"
```

## Implementation
You can create many Manifestos to different scenarios by extending the `TalkerManifesto` class.
To do so, you must override, at least, the following methods:
```cpp
	// The Manifesto class description shouldn't be greater than 42 chars
	// {"m":7,"f":"","s":1,"b":1,"t":"","i":58485,"0":"","1":1,"c":11266} <-- 128 - (66 + 2*10) = 42
	const char* class_description() const override { return "LedManifesto"; }
	const Action* _getActionsArray() const override { return actions; }
    uint8_t _actionsCount() const override { return sizeof(actions)/sizeof(Action); }
```
As it's possible to be seen, these methods relate to the member variable `actions`,
that depending on the number of the actions, it shall follow the following structure:
```cpp
    const Action actions[3] = {
		{"on", "Turns led ON"},
		{"off", "Turns led OFF"},
		{"actions", "Total of triggered Actions"}
    };
```
This structure is a list of `Action` objects with a name and a description of the respective `Action`.
Their `index` is given by the respective position in the array. You must make sure the number of actions is typed,
in the case above, that is represented with `[3]`.

### Example
Here is a bare minimum example of such implementation that controls a Blue LED:
```cpp
#ifndef BLUE_MANIFESTO_HPP
#define BLUE_MANIFESTO_HPP

#include <TalkerManifesto.hpp>

#ifndef LED_BUILTIN	// For the case of an ESP32 board without the LED_BUILTIN defined
  #define LED_BUILTIN 2  // Fallback definition if not already defined
#endif


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

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE CORRECT!

	// The Action pair name and description shouldn't be greater than 40 chars
	// {"m":7,"b":1,"i":6442,"f":"","t":"","0":255,"1":"","2":"","c":25870} <-- 128 - (68 + 2*10) = 40

	// ------------- MAXIMUM SIZE RULER --------------|
	//	 "name", "123456789012345678901234567890123456"
    const Action actions[3] = {
		{"on", "Turns led ON"},
		{"off", "Turns led OFF"},
		{"state", "The actual state of the led"}
    };
    
    bool _is_led_on = false;	// keep track of the led state, by default it's off

public:
    
    const Action* _getActionsArray() const override { return actions; }
    uint8_t _actionsCount() const override { return sizeof(actions)/sizeof(Action); }


    // Index-based operations (simplified examples)
    bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
        (void)talker;		// Silence unused parameter warning
    	(void)talker_match;	// Silence unused parameter warning
		
		// Actual implementation would do something based on index
		switch(index) {

			case 0:
			{
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
```

## Other methods
You can go beyond the mandatory bare minimum above, here are more methods that can be overridden.
### _loop
The `_loop` method is called constantly by the `MessageRepeater` class, so, you can program it
in the same fashion as you would for the typical `loop` function in an Arduino sketch, but specifically for the Talker.

Here is an example of such programming, a *buzzer* that takes its time buzzing and by using the loop
avoids the usage of any `delay` function that interrupt the normal flow of the program.
```cpp
	void _loop(JsonTalker& talker) override {
        (void)talker;		// Silence unused parameter warning
		if ((uint16_t)millis() - _buzz_start > _buzz_duration_ms) {
			#ifdef BUZZ_PIN
			digitalWrite(BUZZ_PIN, LOW);
			#endif
		}
	}
```
The `_loop` method is being run each time the Arduino `loop` function runs given the line bellow, so, one to one run.
```cpp
void loop() {
	message_repeater.loop();	// Keep calling the Message Repeater
}
```

### _echo
The `_echo` method is used to process the message responses, echoes, to the original ones sent.
So, a talker not only is able to receive commands as also is able to send their own generated
by the Talker Manifesto, and thus, with this method, able to process the respective responses, as echoes, in this method.

Note that *commands* are all non response messages, so *commands* exclude `echo`, `error` and obviously `noise`.

Here is an example of a Manifesto that processes the echoes to its generated pings.
```cpp
    void _echo(JsonTalker& talker, JsonMessage& json_message, MessageValue message_value, TalkerMatch talker_match) override {
		(void)talker_match;	// Silence unused parameter warning
        (void)message_value;	// Silence unused parameter warning
		
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
```

### _error
The `_error` method can be used for report the errors returned by other Talkers,
in this example the Talker Manifesto results in the printing of those errors received.
```cpp
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
```
There are many errors types you can use in your specific implementation, here is the full `enum` of error types:
```cpp
    /**
     * @enum ErrorValue
     * @brief Specific error conditions in communication
     * 
     * Detailed error codes for diagnosing communication
     * failures and malformed messages.
     */
    enum ErrorValue : uint8_t {
        TALKIE_ERR_CHECKSUM,  ///< Message checksum failure
        TALKIE_ERR_MESSAGE,   ///< Malformed message structure
        TALKIE_ERR_IDENTITY,  ///< Invalid sender/receiver identity
        TALKIE_ERR_FIELD,     ///< Missing or invalid field
        TALKIE_ERR_FROM,      ///< Invalid source specification
        TALKIE_ERR_TO,        ///< Invalid destination specification
        TALKIE_ERR_DELAY,     ///< Timing/delay violation
        TALKIE_ERR_KEY,       ///< Invalid message key
        TALKIE_ERR_VALUE,     ///< Invalid message value
        TALKIE_ERR_MISSING    ///< Missing message
    };
```

### _noise
The `_noise` as it implies processes messages that lost their usual meaning, this is, their usual meaning was
removed and become noise. One typical scenario is a recovery message that had a bad checksum and thus becomes 'noisy',
this gives the opportunity to the Talker post process it in the method below. You can catch others recovery calls in this
method too, simple because recovery messages are noisy messages.

This is an example that just prints the first value of the 'noisy' message.
```cpp
    void _noise(JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
		(void)talker;		// Silence unused parameter warning
		(void)talker_match;	// Silence unused parameter warning

		if (json_message.is_recover_message()) {
			char from_name[TALKIE_MAX_LEN];
			json_message.get_from_name(from_name);
			Serial.print("Recovery message from: ");
			Serial.print(from_name);
			Serial.print(" - ");
			json_message.write_to(Serial);
			Serial.print(" | ");
			Serial.println((int)json_message.get_recover_message_value());
		}
    }
```
