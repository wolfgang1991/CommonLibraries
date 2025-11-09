#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <ostream>
#include <iostream>
#include <vector>

template <typename T>
class Histogram {

	T minValue;
	T maxValue;
	size_t numBins;
	std::vector<size_t> bins;

	public:

	// Add a value to the histogram (increments the appropriate bin)
	void add(T value) {
		if (numBins == 0) return;
		bins[indexFor(value)]++;
	}

	// Get count for a bin
	size_t getBinCount(size_t binIndex) const {
		if (binIndex >= numBins) return 0;
		return bins[binIndex];
	}

	// Total count across all bins
	size_t totalCount() const {
		size_t sum = 0;
		for (size_t c : bins) sum += c;
		return sum;
	}

	// Reset all bins to zero
	void reset() {
		std::fill(bins.begin(), bins.end(), 0);
	}

	// Return reference to internal bin storage
	const std::vector<size_t>& data() const { return bins; }

	// Compute bin index for a value (clamped into [0, numBins-1])
	size_t indexFor(T value) const {
		if(numBins == 0) return 0;
		if(maxValue == minValue) return 0;
		if(value <= minValue) return 0;
		if(value >= maxValue) return numBins - 1;
		return (value-minValue)*numBins/(maxValue-minValue);//works for integers too
	}

	// Lower bound of a bin (inclusive)
	T binLowerBound(size_t binIndex) const {
		if (numBins == 0) return minValue;
		if (binIndex >= numBins) binIndex = numBins - 1;
		return static_cast<T>(minValue + (maxValue - minValue) * binIndex / numBins);
	}

	// Upper bound of a bin (exclusive except for last bin)
	T binUpperBound(size_t binIndex) const {
		if (numBins == 0) return maxValue;
		if (binIndex + 1 >= numBins) return maxValue;
		return binLowerBound(binIndex + 1);
	}

	Histogram(T minValue, T maxValue, size_t numBins)
		: minValue(minValue), maxValue(maxValue), numBins(numBins), bins(numBins, 0) {}

	~Histogram() {}

};

template <typename T>
std::ostream& operator<<(std::ostream& os, const Histogram<T>& h) {
	const auto& bins = h.data();
	size_t n = bins.size();

	os << "Histogram(min=" << h.binLowerBound(0)
	   << ", max=" << h.binUpperBound(n == 0 ? 0 : n - 1)
	   << ", bins=" << n
	   << ", total=" << h.totalCount() << ")\n";

	for (size_t i = 0; i < n; ++i) {
		os << "  [" << h.binLowerBound(i) << ", " << h.binUpperBound(i) << "): "
		   << h.getBinCount(i) << "\n";
	}

	return os;
}

#endif