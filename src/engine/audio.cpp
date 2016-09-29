#include "audio.hpp"
using namespace engine;

freq_sequence::freq_sequence() {}

freq_sequence::freq_sequence(freq_sequence& a)
{
	seq = a.seq;
}

void
freq_sequence::add(freq_sequence& fs, float start, int voice)
{
	for (auto i : fs.seq)
	{
		i.start += (size_t)(44100.f * start);
		i.voice += voice;
		seq.push_back(i);
	}
}

void
freq_sequence::add(int note, size_t sample, float duration, float volume, int voice)
{
	entry ne;
	ne.start = sample;
	ne.duration = (size_t)(44100.f * duration);
	ne.freq = note_freq(note);
	ne.volume = volume * 10000;
	ne.voice = voice;
	seq.push_back(ne);
}

void
freq_sequence::add(int note, float start, float duration, float volume, int voice)
{
	add(note, (size_t)(44100.f * start), duration, volume, voice);
}

void
freq_sequence::append(int note, float duration, float volume, int voice)
{
	size_t start = 0;
	for (auto& i : seq)
	{
		if (i.start + i.duration > start && i.voice == voice)
		{
			start = i.start + i.duration;
		}
	}

	add(note, start, duration, volume, voice);
}

void
freq_sequence::append(freq_sequence& fs, int voice)
{
	size_t start = 0;
	for (auto& i : seq)
	{
		if (i.start + i.duration > start && i.voice == voice)
			start = i.start + i.duration;
	}

	for (auto i : fs.seq)
	{
		i.start += start;
		i.voice += voice;
		seq.push_back(i);
	}
}

freq_sequence
freq_sequence::snip(size_t s_start, size_t s_duration)
{
	freq_sequence retval;
	size_t s_end = s_start + s_duration;
	for (auto& i : seq)
	{
		if (i.start < s_end && i.start + i.duration > s_start)
		{
			entry ne = i;
			if (ne.start < s_start)
			{
				ne.duration = ne.start + ne.duration - s_start;
				ne.start = 0;
			}
			else
				ne.start -= s_start;

			if (ne.start + ne.duration > s_duration)
				ne.duration = s_duration - ne.start;

			retval.seq.push_back(ne);
		}
	}
	return retval;
}


sample_buffer::sample_buffer() {}
sample_buffer::sample_buffer(sample_buffer& m)
{
	samples = m.samples;
}
sample_buffer::sample_buffer(sample_buffer&& m)
{
	samples = std::move(m.samples);
}

sample_buffer&
sample_buffer::operator=(const sample_buffer& r)
{
	samples = r.samples;
	return *this;
}

sample_buffer
sample_buffer::mix(sample_buffer& buf1, sample_buffer& buf2)
{
	sample_buffer retval;
	size_t size = 0;
	if (buf1.samples.size() < buf2.samples.size())
		retval.samples.reserve((size = buf1.samples.size(), size));
	else
		retval.samples.reserve((size = buf2.samples.size(), size));

	for (size_t i = 0; i < size; i++)
		retval.samples.push_back(buf1.samples[i] + buf2.samples[i]);

	return std::move(retval);
}

int
sample_buffer::mix_with(sample_buffer& buf, int pos)
{
	if (samples.size() < buf.samples.size() + pos)
		samples.resize(buf.samples.size() + pos);

	for (size_t i = 0; i < buf.samples.size(); i++)
		samples[i] += buf.samples[i];

	return 0;
}

void
sample_buffer::generate(sample_buffer& buf, int wave, float f, float v, size_t start, size_t duration)
{
	const unsigned int r = 44100;
	const float b = v / r;

	if (buf.samples.size() < start + duration)
		buf.samples.resize(start + duration, 0);

	for (size_t i = start; i < start + duration; i++)
	{
		float sample = 0;

		if (wave == wave_saw)
		{
			sample = v - std::fmodf((b*f*i), v * 2);
		}
		else if (wave == wave_sine)
		{
			const float pi_2 = std::atanf(1) * 8.f;
			sample = std::sinf(pi_2*(f / r)*i)*v;
		}
		else if (wave == wave_triangle)
		{
			sample = std::abs(v - std::fmodf((b*f*i), v * 2));
		}
		else if (wave == wave_noise)
		{
			sample = v - std::fmodf((float)std::rand(), v * 2);
		}
		buf.samples[i] += (int)sample;
	}
}

void
sample_buffer::generate(sample_buffer& buf, int wave, freq_sequence& seq, int voice)
{
	for (auto& i : seq.seq)
	{
		if (i.voice == voice)
			generate(buf, wave, i.freq, i.volume, i.start, i.duration);
	}
}



sample_mix::sample_mix()
{
	c_section = 0;
}

void
sample_mix::add_mix(freq_sequence& seq, int wave, int voice)
{
	channel nchan;
	nchan.seq = &seq;
	nchan.wave = wave;
	nchan.voice = voice;
	channels.push_back(nchan);
}

void
sample_mix::generate_section(size_t start, size_t duration)
{
	for (auto& i : mix.samples)
		i = 0;

	for (auto& i : channels)
	{
		engine::freq_sequence s = i.seq->snip(start, duration);
		sample_buffer::generate(mix, i.wave, s, i.voice);
	}
}

void
sample_mix::setup()
{
	//generate_section();
	//buffer.loadFromSamples(&mix.samples[0], mix.samples.size(), 1, 44100);
	output.setBuffer(buffer);
	output2.setBuffer(buffer);
	++c_section;
}

const std::vector<signed short>&
sample_mix::get_buffer()
{
	return mix.samples;
}

void
sample_mix::play()
{
	if (output.getStatus() == output.Stopped && output2.getStatus() == output2.Stopped)
	{
		buffer.loadFromSamples(&mix.samples[0], mix.samples.size(), 1, 44100);
		if (c_section % 2)
			output2.play();
		else
			output.play();

		++c_section;
	}
}

class teststream : public sf::InputStream
{
	sample_mix* mix;
	size_t loc;
public:
	teststream()
	{
		loc = 0;
	}
	bool open(sample_mix& nmix)
	{
		mix = &nmix;
		return false;
	}
	sf::Int64 read(void* data, sf::Int64 size)
	{
		mix->generate_section(loc, size/2);
		for (size_t i = 0; i < size/2; i++)
		{
			((unsigned short*)data)[i] = mix->get_buffer().at(i);
		}
		loc += size;
		return size;
	}
	sf::Int64 seek(sf::Int64 position)
	{
		loc = position;
		return position;
	}
	sf::Int64 tell()
	{
		return loc;
	}
	sf::Int64 getSize()
	{
		return 10000;
	}
};

void
sample_mix::test_song()
{
	engine::sample_mix gen;

	printf("generate\n");
	std::getchar();

	engine::freq_sequence motif1;
	motif1.append(0, 0.5f, 0.8f);
	motif1.append(4, 0.5f, 0.8f);
	motif1.append(7, 0.5f, 0.8f);
	motif1.append(11, 0.5f, 0.8f);
	motif1.append(12, 0.5f, 0.8f);
	motif1.append(11, 0.5f, 0.8f);
	motif1.append(7, 0.5f, 0.8f);
	motif1.append(4, 0.5f, 0.8f);

	engine::freq_sequence motif2;
	motif2.append(12, 0.5, 0.9);
	motif2.append(11, 0.5, 0.9);
	motif2.append(12, 1, 0.9);

	engine::freq_sequence motif3;
	motif3.append(-17, 4, 1, 2);
	motif3.append(-19, 4, 1, 2);

	motif3.append(-17, 4, 1, 1);
	motif3.append(-19, 4, 1, 1);

	engine::freq_sequence drums1;
	drums1.append(-24, 0.05f, 1, 3);
	drums1.add(-12, 0.5f, 0.05f, 1, 4);
	drums1.add(-24, 0.75f, 0.05f, 1, 3);
	drums1.add(-12, 1.5f, 0.05f, 1, 4);

	engine::freq_sequence seq;

	seq.append(motif1);
	seq.append(0, 4, 1, 1);
	seq.append(-12, 4, 1, 2);

	seq.append(motif1);
	seq.append(2, 4, 1, 1);
	seq.append(-10, 4, 1, 2);

	seq.add(drums1, 4);
	seq.add(drums1, 6);
	seq.add(drums1, 8);
	seq.add(drums1, 10);
	seq.add(drums1, 12);
	seq.add(drums1, 14);

	seq.add(motif3, 8);

	seq.add(motif2, 8);
	seq.add(motif2, 10);
	seq.add(motif2, 12);
	seq.add(motif2, 14);

	gen.add_mix(seq, engine::sample_buffer::wave_saw, 0);
	gen.add_mix(seq, engine::sample_buffer::wave_saw, 1);
	gen.add_mix(seq, engine::sample_buffer::wave_triangle, 2);
	gen.add_mix(seq, engine::sample_buffer::wave_saw, 3);
	gen.add_mix(seq, engine::sample_buffer::wave_noise, 4);

	teststream s;
	s.open(gen);

	sf::Music m;
	m.openFromStream(s);

	printf("play\n");
	std::getchar();

	m.play();
	std::getchar();
}

