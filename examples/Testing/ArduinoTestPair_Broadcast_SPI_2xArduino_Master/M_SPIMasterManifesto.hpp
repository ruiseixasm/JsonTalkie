/**
 * @file    M_SPIMasterManifesto.hpp
 * @author  Rui Seixas Monteiro
 * @brief   A Manifesto intended to test different types of SPI Sockets in the SPI Master side.
 *
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/manifestos
 * @see https://github.com/ruiseixasm/JsonTalkie/tree/main/sockets
 * 
 * Actions:
 *  - Extensive number of tests named and described in the `calls` variable bellow.
 * 
 * Hardware:
 * - Any type of Arduino compatible board will work.
 * 
 * Created: 2026-02-11
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

	// ALWAYS MAKE SURE THE DIMENSIONS OF THE ARRAYS BELOW ARE THE CORRECT!

	// The Action pair name and description shouldn't be greater than 40 chars
	// {"m":7,"b":1,"i":6442,"f":"","t":"","0":255,"1":"","2":"","c":25870} <-- 128 - (68 + 2*10) = 40

	// ------------- MAXIMUM SIZE RULER --------------|
	//	 "name", "123456789012345678901234567890123456"
    Action actions[6] = {
		{"period", "Sets cycle period milliseconds"},
		{"enabled", "Checks, enable or disable cycles"},
		{"calls", "Gets total calls and their echoes"},
		{"burst", "Tests slave, many messages at once"},
		{"spacing", "Burst spacing in microseconds"},
		{"ping", "Ping talkers by name or channel"}
    };
    
	uint16_t _self_blink_time = 0;

	uint32_t _last_blink = 0;
	uint8_t _yellow_led_on = 0;
	uint32_t _cyclic_period_ms = 60;
	bool _cyclic_transmission = true;	// true by default

	JsonMessage _toggle_yellow_on_off{
		MessageValue::TALKIE_MSG_CALL,
		BroadcastValue::TALKIE_BC_LOCAL
	};

	// For calls
    uint16_t _total_calls = 0;
    uint16_t _total_echoes = 0;

	enum BurstState : uint8_t {
		DORMANT,
		WAIT_ECHO,
		BURSTING
	};

	// For ping
	char _original_message_from_name[TALKIE_NAME_LEN] = {'\0'};
	BroadcastValue _original_message_broadcast = BroadcastValue::TALKIE_BC_NONE;
	uint16_t _original_message_id = 0;

	// For burst
	#define burst_amount 20
	uint8_t _burst_toggles = 0;
	uint16_t _start_calls = 0;
	bool _original_cyclic_transmission = true;
	BurstState _burst_state = DORMANT;
	uint32_t _burst_spacing_us = 0;
	uint32_t _last_burst_us = 0;

public:

    const Action* _getActionsArray() const override { return actions; }

    // Size methods
    uint8_t _actionsCount() const override { return sizeof(actions)/sizeof(Action); }

    
    // Index-based operations (simplified examples)
    bool _actionByIndex(uint8_t index, JsonTalker& talker, JsonMessage& json_message, TalkerMatch talker_match) override {
    	(void)talker;	// Silence unused parameter warning
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
				if (json_message.has_nth_value_number(0)) {
					if(json_message.get_nth_value_number(0)) {
						_cyclic_transmission = true;
					} else {
						_cyclic_transmission = false;
					}
				} else {
					json_message.set_nth_value_number(0, (uint32_t)_cyclic_transmission);
				}
				return true;
			break;
				
			case 2:
				if (json_message.has_nth_value_number(0)) {
					_total_calls = 0;
					_total_echoes = 0;
				}
				json_message.set_nth_value_number(0, _total_calls);
				json_message.set_nth_value_number(1, _total_echoes);
				return true;
			break;
			
			case 3:	// Burst
			{
				_original_cyclic_transmission = _cyclic_transmission;
				_cyclic_transmission = false;
				json_message.get_from_name(_original_message_from_name);
				_original_message_broadcast = json_message.get_broadcast_value();
				_original_message_id = json_message.get_identity();
				// Used the Roger message as the echo message avoiding this way
				// the SPI Master losing time with the Serial communication
				json_message.set_message_value(MessageValue::TALKIE_MSG_SYSTEM);
				json_message.set_broadcast_value(BroadcastValue::TALKIE_BC_LOCAL);
				json_message.set_system_value(SystemValue::TALKIE_SYS_CALLS);
				// Because it is NOT an echo message anymore, no swapping of 'from' with 'to' happens
				json_message.set_to_name("slave");
				// No need to transmit the message, the normal ROGER reply does that for us!
				return true;
			}
			break;
			
			case 4:
				if (json_message.has_nth_value_number(0)) {
					_burst_spacing_us = json_message.get_nth_value_number(0);
				} else {
					json_message.set_nth_value_number(0, _burst_spacing_us);
				}
				return true;
			break;
				
			case 5:
			{
				// 1. Start by collecting info from message
				json_message.get_from_name(_original_message_from_name);
				_original_message_broadcast = json_message.get_broadcast_value();
				_original_message_id = json_message.get_identity();

				// 2. Repurpose it to be a LOCAL PING
				json_message.set_message_value(MessageValue::TALKIE_MSG_PING);
				json_message.remove_to();	// MAkes sure To is removed, otherwise it will be sent to me
				// Because it is NOT an echo message anymore, no swapping of 'from' with 'to' happens
				if (json_message.get_nth_value_type(0) == ValueType::TALKIE_VT_STRING) {
					char value_name[TALKIE_NAME_LEN];
					if (json_message.get_nth_value_string(0, value_name, TALKIE_NAME_LEN)) {
						json_message.set_to_name(value_name);
					}
				} else if (json_message.get_nth_value_type(0) == ValueType::TALKIE_VT_INTEGER) {
					json_message.set_to_channel((uint8_t)json_message.get_nth_value_number(0));
				}
				json_message.remove_nth_value(0);

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


	void _loop(JsonTalker& talker) override {
		
		if ((uint16_t)millis() - _self_blink_time >= 50) {	// turns off for 50 milliseconds
			
			digitalWrite(LED_BUILTIN, HIGH);
		}

		if (millis() - _last_blink >= _cyclic_period_ms) {	// Has to be >= to avoid extra millisecond
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

		if (_burst_toggles > 0) {
			if (micros() - _last_burst_us >= _burst_spacing_us) {
				_last_burst_us = micros();
				_burst_toggles--;
				
				if (_yellow_led_on++ % 2) {
					_toggle_yellow_on_off.set_action_name("off");
				} else {
					_toggle_yellow_on_off.set_action_name("on");
				}
				talker.transmitToRepeater(_toggle_yellow_on_off);
			}
		} else if (_burst_state == BURSTING && micros() - _last_burst_us >= 10 * 1000) {	// Give 10 milliseconds to rest
			_burst_state = WAIT_ECHO;
			// Get the SPI Slave actual calls
			JsonMessage get_calls{
				MessageValue::TALKIE_MSG_SYSTEM,
				BroadcastValue::TALKIE_BC_LOCAL
			};
			get_calls.set_system_value(SystemValue::TALKIE_SYS_CALLS);
			// Because it is NOT an echo message anymore, no swapping of 'from' with 'to' happens
			get_calls.set_to_name("slave");
			talker.transmitToRepeater(get_calls);
		}
	}


    void _echo(JsonTalker& talker, JsonMessage& json_message, MessageValue message_value, TalkerMatch talker_match) override {
		(void)talker_match;	// Silence unused parameter warning

		switch (message_value) {
			
			case MessageValue::TALKIE_MSG_CALL:
				digitalWrite(LED_BUILTIN, LOW);
				_self_blink_time = (uint16_t)millis();
				_total_echoes++;
			break;

			case MessageValue::TALKIE_MSG_SYSTEM:
				if (json_message.has_nth_value_number(0)) {
					if (_burst_state == WAIT_ECHO) {
						_burst_state = DORMANT;
						_cyclic_transmission = _original_cyclic_transmission;
						uint16_t received_calls = json_message.get_nth_value_number(0) - _start_calls;
						JsonMessage report_burst{
							MessageValue::TALKIE_MSG_ECHO,
							BroadcastValue::TALKIE_BC_REMOTE
						};
						report_burst.set_broadcast_value(_original_message_broadcast);
						report_burst.set_to_name(_original_message_from_name);
						report_burst.set_from_name(talker.get_name());	// It's an echo
						report_burst.set_identity(_original_message_id);
						report_burst.set_nth_value_number(0, received_calls);
						if (received_calls == burst_amount)	{
							report_burst.set_nth_value_string(1, "OK");
						} else {
							report_burst.set_nth_value_string(1, "FAIL");
						}
						talker.transmitToRepeater(report_burst);
					} else {
						_burst_state = BURSTING;
						_start_calls = json_message.get_nth_value_number(0);
						_burst_toggles = burst_amount;
					}
				}
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
				json_message.set_to_name(_original_message_from_name);
				json_message.set_from_name(talker.get_name());

				// Emulates the REMOTE original call
				json_message.set_identity(_original_message_id);

				// It's already an ECHO message, it's because of that that entered here
				// Finally answers to the REMOTE caller by repeating all other json fields
				json_message.set_broadcast_value(_original_message_broadcast);
				talker.transmitToRepeater(json_message);
			}
			break;

			default: break;
		}
    }

};


#endif // DUO_TESTER_MANIFESTO_HPP
