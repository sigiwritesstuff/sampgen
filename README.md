# sampgen
C library to generate float audio samples from breakpoint files formatted to give timestamp, frequency, amplitude and options to slur notes/add a glissando. It includes functions to generate samples as triangle/square/sawtooth/sine waves, two envelopes, and to read frequencies as Hz or convert from MIDI-style note numbers to n-tet, kirnberger 3 or enter tunings for pelog/slendro scales of Javanese gamelan music. It was made using example code on envelopes, wavetable synthesis and building on breakpoint file formats/functions reading from them in The Audio Programming Book(2010) by Richard Boulanger, Victor Lazzarini et al in order to work as a unified interface, with the ability to adding new waveform/envelope/tuning system functions externally and with multiple tunings for breakpoint frequency data.# sampgen
