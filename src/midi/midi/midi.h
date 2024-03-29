
#ifndef MIDI_H
#define MIDI_H

#include <cstdint>
#include <ostream>
#include <functional>
#include <vector>
#include "primitives.h"

namespace midi {

	struct CHUNK_HEADER {
		char id[4];
		uint32_t size;
	};

	void read_chunk_header(std::istream& , CHUNK_HEADER*);

	std::string header_id(CHUNK_HEADER);

#pragma pack(push,1)
	struct MTHD {
		CHUNK_HEADER header;
		uint16_t type;
		uint16_t ntracks;
		uint16_t division;
	};
#pragma pack(pop)

	void read_mthd(std::istream&, MTHD*);

	bool is_sysex_event(uint8_t byte);
	bool is_meta_event(uint8_t byte);
	bool is_midi_event(uint8_t byte);
	bool is_running_status(uint8_t byte);

	uint8_t extract_midi_event_type(uint8_t status);
	Channel extract_midi_event_channel(uint8_t status);


	bool is_note_off(uint8_t status);
	bool is_note_on(uint8_t status);
	bool is_polyphonic_key_pressure(uint8_t status);
	bool is_control_change(uint8_t status);
	bool is_program_change(uint8_t status);
	bool is_channel_pressure(uint8_t status);
	bool is_pitch_wheel_change(uint8_t status);

	struct EventReceiver {
		
		virtual void note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) = 0;
		virtual void note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) = 0;
		virtual void polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure) = 0;
		virtual void control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value) = 0;
		virtual void program_change(Duration dt, Channel channel, Instrument program) = 0;
		virtual void channel_pressure(Duration dt, Channel channel, uint8_t pressure) = 0;
		virtual void pitch_wheel_change(Duration dt, Channel channel, uint16_t value) = 0;
		virtual void meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size) = 0;
		virtual void sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size) = 0;
	};

	void read_mtrk(std::istream&, EventReceiver&);

	struct NOTE {
		NoteNumber note_number;
		Time start;
		Duration duration;
		uint8_t velocity;
		Instrument instrument;

		NOTE(NoteNumber note_number, Time start, Duration duration, uint8_t velocity, Instrument instrument) :
			note_number(note_number), start(start), duration(duration), velocity(velocity), instrument(instrument) {

		}

		bool operator ==(const NOTE& other) const;
		bool operator !=(const NOTE& other) const;
	};

	std::ostream& operator <<(std::ostream& out, const NOTE& note);

	struct ChannelNoteCollector : EventReceiver {
		Channel channel;
		std::function<void(const NOTE&)> noteReceiver;
		Instrument instrument = Instrument(0);
		Time time = Time(0);
		Time timeArray[128];
		uint16_t snelheden[128];

		ChannelNoteCollector(Channel ch,
			std::function<void(const NOTE&)> noteReceiver) :
			channel(ch), noteReceiver(noteReceiver) {

			for (uint16_t& v : snelheden) {
				v = 128;
			}
		}

		
		virtual void note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		virtual void note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		virtual void polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure) override;
		virtual void control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value) override;
		virtual void program_change(Duration dt, Channel channel, Instrument program) override;
		virtual void channel_pressure(Duration dt, Channel channel, uint8_t pressure) override;
		virtual void pitch_wheel_change(Duration dt, Channel channel, uint16_t value) override;
		virtual void meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
		virtual void sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
	};


	struct EventMulticaster : public EventReceiver {
		std::vector<std::shared_ptr<EventReceiver>> receivers;

		EventMulticaster(std::vector<std::shared_ptr<EventReceiver>> receivers) :
			receivers(receivers) { };

		virtual void note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		virtual void note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		virtual void polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure) override;
		virtual void control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value) override;
		virtual void program_change(Duration dt, Channel channel, Instrument program) override;
		virtual void channel_pressure(Duration dt, Channel channel, uint8_t pressure) override;
		virtual void pitch_wheel_change(Duration dt, Channel channel, uint16_t value) override;
		virtual void meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
		virtual void sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
	};

	class NoteCollector : public EventReceiver
	{
	public:
		EventMulticaster multicaster;
		std::function<void(const NOTE&)> receiver;

		static std::vector<std::shared_ptr<EventReceiver>>
			create_list(std::function<void(const NOTE&)> receiver)
		{
			std::vector<std::shared_ptr<EventReceiver>> receivers;
			for (int channel = 0; channel < 16; channel++)
			{
				auto ptr = std::make_shared<ChannelNoteCollector>
					(Channel(channel), receiver);
				receivers.push_back(ptr);
			}
			return receivers;
		}

		NoteCollector(std::function<void(const NOTE&)> receiver) :
			receiver(receiver), multicaster(create_list(receiver)) { }

		void note_on(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		void note_off(Duration dt, Channel channel, NoteNumber note, uint8_t velocity) override;
		void polyphonic_key_pressure(Duration dt, Channel channel, NoteNumber note, uint8_t pressure) override;
		void control_change(Duration dt, Channel channel, uint8_t controller, uint8_t value) override;
		void program_change(Duration dt, Channel channel, Instrument program) override;
		void channel_pressure(Duration dt, Channel channel, uint8_t pressure) override;
		void pitch_wheel_change(Duration dt, Channel channel, uint16_t value) override;
		void meta(Duration dt, uint8_t type, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;
		void sysex(Duration dt, std::unique_ptr<uint8_t[]> data, uint64_t data_size) override;

	};

	std::vector<NOTE> read_notes(std::istream&);
}
#endif // !MIDI_H
