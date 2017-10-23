
#include <array>
#include <limits>
#include <vector>

#include "fixed_point.h"

namespace audio {
namespace {

// The first four bits of a byte, signed, and sign extended through the byte
// if negative. 
typedef int8_t LoNibble;

typedef std::vector<int16_t>::const_iterator SampleConstIter;

typedef std::array<int8_t, 8>::iterator BRRBlockSampleIter;
typedef std::vector<int8_t>::iterator BRRSampleIter;

struct BRRFilterLPConstants {
  fix16_16 k1;
  fix16_16 k2;
};

// BRR Filter constants. We use those used by the SNES in an effort to mimic
// its characteristic sound.
const BRRFilterLPConstants kBRRFilterCoefficients[] = {
  { 0, 0 },
  { 61440, 0 },
  { 124928, 61440 },
  { 117760, 53248 } };

struct BRRFilter {
  // Breaks a filter_byte apart into the filter constants.
  explicit BRRFilter(int8_t filter_byte) : exp_bit_shift(filter_byte & 0x0f),
      lp_k(kBRRFilterCoefficients[(filter_byte & 0x30) >> 4]) {}
  // Represents a div/mul by a power of 2 by a bit shift amount.
  int exp_bit_shift;
  // 16.16 fixed point filter coefficients.
  BRRFilterLPConstants lp_k;
};

// Individual BRR compression/decompression methods. The d_ in d_sample_minus_*
// is meant to signify that these expect comp->decomp samples (see
// GreedyBRRCompress docs to learn why).
LoNibble BRRCompressSample(int16_t sample_0, int16_t d_sample_minus_1,
    int16_t d_sample_minus_2, BRRFilter filter);
int16_t BRRDecompressSample(LoNibble sample_0, int16_t d_sample_minus_1,
    int16_t d_sample_minus_2, BRRFilter filter);

// BRR compress a sample, using the previous comp->decomp samples and BRR 
// filter into a result nibble. [*decompressed_result] becomes the comp->decomp
// result of this sample.
//
// Returns the absolute difference between the original sample and the new
// value of [*decompressed_result].
int32_t BRRCompressAndGetError(int16_t sample_0, int16_t d_sample_minus_1,
    int16_t d_sample_minus_2, BRRFilter filter, LoNibble* result,
    int16_t* decompressed_result) {
  *result =
      BRRCompressSample(sample_0, d_sample_minus_1, d_sample_minus_2, filter);
  // NOTE: *result is really a signed byte at this point wrapped to nibble
  // range
  *decompressed_result = BRRDecompressSample(*result, d_sample_minus_1,
      d_sample_minus_2, filter);
  return abs(static_cast<int32_t>(sample_0) - *decompressed_result);
}

// Compress 16 16b samples to 9 4b compressed values using BRR.
//
// This method will compress [samples] samples in [sample_data] to
// [compressed_data] provided: the 2 compressed->decompressed (see comment on 
// why we use samples computed this way over just the raw samples) samples
// previous to the ones in [sample_data], the BRR filter to use, and a location
// to write the compressed data. 
//
// We set [decomp_samples] to be the last 2 compressed->decompressed samples.
// decomp_samples[0] is older than decomp_samples[1].
//
// Returns the absolute difference between the comp->decomp samples and the 
// original samples.
//
// ** WHY WE PASS AROUND COMP->DECOMP PREVIOUS SAMPLES INSTEAD OF JUST USING
//    RAW PREVIOUS SAMPLES **
//
// When compressing a value, we use the result of decompressing the previous
// compressed samples to do so instead of using just the original previous
// samples. Theoretically, this should bring a decompressed sample closer to
// its original value as playback decompression ALSO (by virtue of not having
// the original sample data) will use decompressed values. 
//
// ** WHY GREEDY **
//
// Instead of choosing values that minimize the absolute error between the
// original sample data and comp->decomp samples, we greedily pick compressed
// values by simply reversing the compression function sample-per-sample. The
// hope is that this less-accurate method of compression will lead to "dirtier"
// sounding audio when decompressed. Dirty in the good way >:)
//
int32_t GreedyBlockBRRSampleCompress(
    SampleConstIter sample_data, 
    int16_t d_sample_minus_1,
    int16_t d_sample_minus_2,
    int samples,
    BRRFilter filter,
    BRRBlockSampleIter compressed_data,
    std::array<int16_t, 2>* last_decomp_samples) {
  int32_t error = 0;
  LoNibble compressed_sample;
  // A slight name change, but they mean the same thing, we just opt to strip
  // having to dereference the function parameter version.
  std::array<int16_t, 2>& prev_decomp_samples = *last_decomp_samples;
  
  error += BRRCompressAndGetError(*sample_data, d_sample_minus_1,
      d_sample_minus_2, filter, &compressed_sample,
      &prev_decomp_samples[0]);
  // We erase any 1's in the high nibble so we can safely or-in a value to
  // *compressed_data later.
  *compressed_data = compressed_sample & 0x0f;
  if (samples == 1) return error;
  ++sample_data;

  error += BRRCompressAndGetError(*sample_data, prev_decomp_samples[0],
      d_sample_minus_1, filter, &compressed_sample,
      &prev_decomp_samples[1]);
  // This or-ing is safe becase: (see comment on previous *compressed_data = )
  *compressed_data |= compressed_sample << 4;
  if (samples == 2) return error;
  ++sample_data;

  ++compressed_data;
  // 0 based "current sample out of [samples]" counter. We initialize it to 1
  // as the while loop sentinel will pre-increment it to 2 before it checks
  // the bounds condition on the first iteration. As making-it-this-far means
  // we MUST observe an iteration of the loop, we must set it this way.
  int sample_counter = 1;
  // 0 if the current compressed sample is at a low nibble, and 1 if its at
  // a high nibble.
  int sample_counter_lo_bit;
  while (++sample_counter < samples) {
    compressed_sample = 0;

    int16_t decomp_sample;
    error += BRRCompressAndGetError(*sample_data, prev_decomp_samples[1],
        prev_decomp_samples[0], filter, &compressed_sample, &decomp_sample);
    
    // We erase any 1's in the high nibble so we can safely or-in a value to
    // *compressed_data later.
    compressed_sample &= 0x0f;

    // Shift down our decompressed samples.
    prev_decomp_samples[0] = prev_decomp_samples[1];
    prev_decomp_samples[1] = decomp_sample;

    sample_counter_lo_bit = sample_counter & 1;
    // Clear the current compressed_data byte if we're writing to it for the 
    // first time (so we're writing the low nibble).
    *compressed_data &= ~(0xff << (sample_counter_lo_bit * 8));
    // Or-in the low or high nibble in *compressed_data as
    // sample_counter_lo_bit dictates. We can safely "or" as we always clear 
    // *compressed_data in the previous line before we write the low nibble. 
    *compressed_data |= compressed_sample << (sample_counter_lo_bit * 4);
    // If we just wrote a high nibble, advance compressed_data to the next 
    // byte.
    compressed_data += sample_counter_lo_bit;
    ++sample_data;
  }
  return error;
}

// Reverse the linear prediction formula:
//
//    c = clamp((s_n - k1 * s_n-1 + k2 * s_n-2) / 2^r, -8, 7)
//
// Where c is the compressed nibble, s are the decompressed samples, and n is 
// the current sample.
//
LoNibble BRRCompressSample(int16_t sample_0, int16_t d_sample_minus_1,
    int16_t d_sample_minus_2, BRRFilter filter) {
  return std::clamp(((static_cast<int32_t>(sample_0) << 16) -
      filter.lp_k.k1*d_sample_minus_1 + filter.lp_k.k2*d_sample_minus_2) >>
      (filter.exp_bit_shift + 16), -8, 7);
}
// Apply the linear prediction formula:
//
//    s_n = 2^r * c + k1 * s_n-1 - k2 * s_n-2 
//
// Where c is the compressed nibble, s are the decompressed samples, and n is
// the current sample.
//
int16_t BRRDecompressSample(LoNibble sample_0, int16_t d_sample_minus_1,
    int16_t d_sample_minus_2, BRRFilter filter) {
  return static_cast<int16_t>(((static_cast<int32_t>(sample_0) <<
      (filter.exp_bit_shift + 16)) + filter.lp_k.k1*d_sample_minus_1 -
      filter.lp_k.k2*d_sample_minus_2) >> 16);
}

// BRR Compress a single block of up to 16 samples.
//
// This function will try all 64 filter modes and pick the mode that results
// in the least error between the raw and comp->decomp data.
void BlockBRRCompress(
    SampleConstIter sample_data,
    int samples,
    BRRSampleIter compressed_data,
    std::array<int16_t, 2>* last_decomp_samples) {
  // Save these values so we can write over decomp_samples.
  const int16_t d_sample_minus_1 = (*last_decomp_samples)[1];
  const int16_t d_sample_minus_2 = (*last_decomp_samples)[0];

  std::array<int16_t, 2> current_last_decomp_samples;
  std::array<int8_t, 8> current_compressed_data;
  int32_t current_error;
  int32_t best_error = std::numeric_limits<int32_t>::max();

  // The filter mode resulting in the least error.
  int8_t best_filter = 0;

  // Try all 64 filter modes; pick the one that minimizes the error. This makes
  // no attempt to order the filter modes to avoid extra copies. Also, theres
  // no real reason we have to do this, but this is in no way performance
  // critical so we opt for ease of ... understand...ment...?
  for (int8_t filter_mode = 0; filter_mode < 63; ++filter_mode) {
    BRRFilter filter(filter_mode);
    // If the compression with this filter resulted in a smaller-than-current
    // error...
    current_error = GreedyBlockBRRSampleCompress(sample_data, d_sample_minus_1,
        d_sample_minus_2, samples, filter, current_compressed_data.begin(),
        &current_last_decomp_samples);
    if (current_error < best_error) {
      best_error = current_error;
      best_filter = filter_mode;
    }
  }

  // Run the compression with the winning filter mode on this block.
  BRRFilter filter(best_filter);
  (void) GreedyBlockBRRSampleCompress(sample_data, d_sample_minus_1,
      d_sample_minus_2, samples, filter, current_compressed_data.begin(),
      &current_last_decomp_samples);

  // How many bytes (including potentially only half of the last one) will our
  // compressed data occupy
  int copy_bytes = samples / 2;
  copy_bytes += copy_bytes & 1;

  std::copy(current_compressed_data.begin(), 
      current_compressed_data.begin() + copy_bytes, compressed_data + 1);

  // Copy out the last two comp->decomp samples.
  std::copy(current_last_decomp_samples.begin(),
      current_last_decomp_samples.end(), last_decomp_samples->begin());

  *compressed_data = best_filter;
}

LoNibble SignExtendNibble(LoNibble x) {
  // Sign extend sample_0 so that int casting will treat it as a signed byte
  // were it a signed nibble.
  return x | (0xf0 * ((x >> 3) & 1));
}
} // namespace

std::vector<int8_t> BRRCompress(const std::vector<int16_t>& sample_data) {
  std::vector<int8_t> comp_data;

  // To begin, we pad the last two comp->decomp samples before the current
  // sample with 0's
  std::array<int16_t, 2> last_decomp_samples = {0, 0};
  // 16 samples at a time...
  for (size_t sample_data_start = 0; 
      sample_data_start < sample_data.size();
      sample_data_start += 16) {
    
    // We compress 16 total samples at a time, or, however many we have
    // left.
    size_t sample_end = sample_data_start + 16;
    int total_samples = static_cast<int>((sample_end > sample_data.size()) ? 
        (sample_data.size() - sample_data_start) : 16);
    
    // How many bytes (rounded up to be even since each byte stores 2 samples),
    // will our compressed data occupy PLUS the filter.
    int compressed_bytes = total_samples / 2;
    compressed_bytes = (compressed_bytes + (compressed_bytes & 1)) + 1;
    comp_data.resize(comp_data.size() + compressed_bytes);

    BlockBRRCompress(sample_data.begin() + sample_data_start, total_samples, 
      comp_data.end() - compressed_bytes, &last_decomp_samples);
  }
  return comp_data;
}

std::vector<int16_t> BRRDecompress(const std::vector<int8_t>& comp_data) {
  std::vector<int16_t> sample_data;
  // The current output sample we're processing.
  size_t cur_sample = 0;

  // To begin, we pad the last two comp->decomp samples before the current
  // sample with 0's
  std::array<int16_t, 2> last_decomp_samples = {0, 0};
  // 9 compressed bytes at a time...
  for (size_t comp_data_start = 0;
    comp_data_start < comp_data.size();
    comp_data_start += 9) {

    size_t comp_data_end = comp_data_start + 9;
    // The actual samples can only occupy up to 8 bytes from the block we're
    // about to decompress, so we subtract 1.
    int total_bytes = static_cast<int>(
      (comp_data_end > comp_data.size()) ?
      (comp_data.size() - comp_data_start) : 9) - 1;
    // If total_bytes is less than 8, we'll change the value of comp_data_end
    // here.
    comp_data_end = comp_data_start + total_bytes + 1;

    // Since we pack two samples in to every byte, resize byte total_bytes * 2
    sample_data.resize(sample_data.size() + total_bytes * 2);
    
    // In a BRR block, the first byte is the filter mode, the remaining bytes
    // each hold two sample nibbles.
    BRRFilter filter(comp_data[comp_data_start]);
    for (size_t cur_byte_index = comp_data_start + 1;
        cur_byte_index < comp_data_end;
        ++cur_byte_index) {
      int8_t cur_byte = comp_data[cur_byte_index];
      // Decompress each nibble using the filter.
      sample_data[cur_sample] = BRRDecompressSample(
          SignExtendNibble(cur_byte & 0x0f), last_decomp_samples[1],
          last_decomp_samples[0], filter);
      // NOTE: We don't sign extend this nibble here as a right shift will
      // automatically sign extend if negative.
      sample_data[cur_sample + 1] = BRRDecompressSample(cur_byte >> 4,
          sample_data[cur_sample], last_decomp_samples[1], filter);

      last_decomp_samples[0] = sample_data[cur_sample];
      last_decomp_samples[1] = sample_data[cur_sample + 1];
      cur_sample += 2;
    }
  }
  return sample_data;
}

} // namespace audio