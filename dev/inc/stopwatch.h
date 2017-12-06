#ifndef UTIL_STOPWATCH_H_
#define UTIL_STOPWATCH_H_

namespace util {

// A Stopwatch... it's a stopwatch.
class Stopwatch {
 public:
  virtual ~Stopwatch() {}
  
  // Return the number of elapsed seconds since the last time Lap was called,
  // or this stopwatch was constructed, whichever is more recent.
  virtual double Lap() = 0;
};

class RealStopwatch : public Stopwatch {
public:
  RealStopwatch();
  
  double Lap() override;

  // Return the smallest unit of time in seconds measurable by this stopwatch.
  static double Precision();
private:
  uint64_t last_ticks_;
};

} // namespace util


#endif // UTIL_STOPWATCH_H_