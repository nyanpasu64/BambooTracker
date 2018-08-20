#pragma once

#include <vector>
#include <memory>
#include "pattern.hpp"
#include "misc.hpp"

struct TrackAttribute;
struct OrderData;

class Track
{
public:
	Track(int number, SoundSource source, int channelInSource);
	TrackAttribute getAttribute() const;
	std::vector<int> getOrderList() const;
	OrderData getOrderData(int order);
	Pattern& getPattern(int num);
	Pattern& getPatternFromOrderNumber(int num);

	void registerPatternToOrder(int order, int pattern);
	void insertOrderBelow(int order);
	void deleteOrder(int order);

private:
	std::unique_ptr<TrackAttribute> attrib_;

	std::vector<int> order_;
	std::vector<Pattern> patterns_;
};

struct TrackAttribute
{
	int number;
	SoundSource source;
	int channelInSource;
};


struct OrderData
{
	TrackAttribute trackAttribute;
	int order;
	int patten;
};
