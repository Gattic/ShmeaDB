// Copyright 2020 Robert Carneiro, Derek Meer, Matthew Tabak, Eric Lujan, Kevin Ko
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
// associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#include "GAnalysis.h"
#include "GList.h"
#include "GTable.h"
#include "GType.h"
#include <iostream>

using namespace shmea;

/*!
 * @brief GAnalysis default constructor
 * @details creates a GAnalysis constructor with no parameters
 */
GAnalysis::GAnalysis(const GTable& newData)
{
	inputData = newData;
}
/*!
 * @brief get result
 * @details returns inputData, presumably after a number of Apply() calls
 * @return the inputData field
 */
GTable GAnalysis::getResult()
{
	return inputData;
}

void GAnalysis::Apply(unsigned int indicator, unsigned int period)
{
	if (inputData.numberOfRows() == 0 || inputData.numberOfCols() == 0)
		return;

	GList temp = inputData.getCol(1);
	GList temp_close = inputData.getCol(2);
	GList temp_high = inputData.getCol(3);
	GList temp_low = inputData.getCol(4);
	GList temp_volume = inputData.getCol(5);

	std::vector<float> temp_num;
	std::vector<float> temp_num_close;
	std::vector<float> temp_num_high;
	std::vector<float> temp_num_low;
	std::vector<float> temp_num_volume;
	std::vector<float> temp_num2;
	std::vector<float> temp_num2_valid;

	GList temp2;
	GList temp2_valid;

	for (unsigned int q = 0; q < temp.size(); ++q)
	{
		temp_num.push_back(temp.getDouble(q));
		temp_num_close.push_back(temp.getDouble(q));
		temp_num_high.push_back(temp.getDouble(q));
		temp_num_low.push_back(temp.getDouble(q));
		temp_num_volume.push_back(temp.getDouble(q));
	}

	if (indicator == 0)
	{
		temp_num2 = RateofChange(temp_num, period);

		for (unsigned int e = 0; e < period - 1; ++e)
		{
			temp2_valid.addLong(0l);
			temp2.addFloat(0x7F80000000000000);
		}

		for (unsigned int w = 0; w < temp_num2.size(); ++w)
		{
			temp2_valid.addLong(1l);
			temp2.addGType(temp_num2[w]);
		}

		inputData.addCol("ROC_ind", temp2_valid);
		inputData.addCol("ROC", temp2);
	}

	else if (indicator == 1)
	{
		temp_num2 = SimpleMovingAverage(temp_num, period);

		for (unsigned int e = 0; e < period - 1; ++e)
		{
			temp2_valid.addLong(0l);
			temp2.addFloat(0x7F80000000000000);
		}

		for (unsigned int w = 0; w < temp_num2.size(); ++w)
		{
			temp2_valid.addLong(1l);
			temp2.addGType(temp_num2[w]);
		}

		inputData.addCol("SMA_ind", temp2_valid);
		inputData.addCol("SMA", temp2);
	}

	else if (indicator == 2)
	{
		temp_num2 = ExpMovingAverage(temp_num, period);

		for (unsigned int e = 0; e < period - 1; ++e)
		{
			temp2_valid.addLong(0l);
			temp2.addFloat(0x7F80000000000000);
		}

		for (unsigned int w = 0; w < temp_num2.size(); ++w)
		{
			temp2_valid.addLong(1l);
			temp2.addGType(temp_num2[w]);
		}

		inputData.addCol("EMA_ind", temp2_valid);
		inputData.addCol("EMA", temp2);
	}

	else if (indicator == 3)
	{
		temp_num2 = RSI(temp_num, period);

		for (unsigned int e = 0; e < period - 1; ++e)
		{
			temp2_valid.addLong(0l);
			temp2.addFloat(0x7F80000000000000);
		}

		for (unsigned int w = 0; w < temp_num2.size(); ++w)
		{
			temp2_valid.addLong(1l);
			temp2.addGType(temp_num2[w]);
		}

		inputData.addCol("RSI_ind", temp2_valid);
		inputData.addCol("RSI", temp2);
	}

	else if (indicator == 5)
	{
		temp_num2 = MFI(temp_num_high, temp_num_low, temp_num_close, temp_num_volume, period);

		for (unsigned int e = 0; e <= period - 1; ++e)
		{
			temp2_valid.addLong(0l);
			temp2.addFloat(0x7F80000000000000);
		}

		for (unsigned int w = 0; w < temp_num2.size(); ++w)
		{
			temp2_valid.addLong(1l);
			temp2.addGType(temp_num2[w]);
		}

		inputData.addCol("MFI_ind", temp2_valid);
		inputData.addCol("MFI", temp2);
	}

	else if (indicator == 6)
	{
		temp_num2 = Stochastic(temp_num_high, temp_num_low, temp_num_close, period);

		for (unsigned int e = 0; e < period - 1; ++e)
		{
			temp2_valid.addLong(0l);
			temp2.addFloat(0x7F80000000000000);
		}

		for (unsigned int w = 0; w < temp_num2.size(); ++w)
		{
			temp2_valid.addLong(1l);
			temp2.addGType(temp_num2[w]);
		}

		inputData.addCol("Stochastic_ind", temp2_valid);
		inputData.addCol("Stochastic", temp2);
	}

	else if (indicator == 7)
	{
		temp_num2 = VWAP(temp_num_high, temp_num_low, temp_num_close, temp_num_volume, period);

		for (unsigned int e = 0; e < period - 1; ++e)
		{
			temp2_valid.addLong(0l);
			temp2.addFloat(0x7F80000000000000);
		}

		for (unsigned int w = 0; w < temp_num2.size(); ++w)
		{
			temp2_valid.addLong(1l);
			temp2.addGType(temp_num2[w]);
		}

		inputData.addCol("VWAP_ind", temp2_valid);
		inputData.addCol("VWAP", temp2);
	}

	else if (indicator == 8)
	{
		temp_num2 = WilliamsR(temp_num_high, temp_num_low, temp_num_close, period);

		for (unsigned int e = 0; e < period - 1; ++e)
		{
			temp2_valid.addLong(0l);
			temp2.addFloat(0x7F80000000000000);
		}

		for (unsigned int w = 0; w < temp_num2.size(); ++w)
		{
			temp2_valid.addLong(1l);
			temp2.addGType(temp_num2[w]);
		}

		inputData.addCol("WilliamsR_ind", temp2_valid);
		inputData.addCol("WilliamsR", temp2);
	}
}

void GAnalysis::Apply(unsigned int indicator)
{
	if (inputData.numberOfRows() == 0 || inputData.numberOfCols() == 0)
		return;

	GList temp = inputData.getCol(1);
	GList temp1 = inputData.getCol(5);
	std::vector<float> temp_num;
	std::vector<float> temp_num2;
	std::vector<float> temp_num3;
	GList temp2;

	for (unsigned int q = 0; q < temp.size(); ++q)
	{
		temp_num.push_back(temp.getDouble(q));
		temp_num3.push_back(temp1.getDouble(q));
	}

	if (indicator == 4)
	{
		temp_num2 = OBV(temp_num, temp_num3);

		for (unsigned int w = 0; w < temp_num2.size(); ++w)
			temp2.addGType(temp_num2[w]);

		inputData.addCol("OBV", temp2);
	}
}

/*!
 * @brief calculates SMA
 * @details Simple Moving Average calculation
 * @param input the stock prices in a vector
 * @param period the length of a period
 */
std::vector<float> GAnalysis::SimpleMovingAverage(const std::vector<float>& input,
												  unsigned int period)
{
	std::vector<float> avg;
	float sum = 0;
	unsigned int len = input.size();
	for (unsigned int i = 0; i <= len - period; ++i)
	{
		for (unsigned int j = i; j < i + period; ++j)
		{
			sum = sum + input[j];
		}
		avg.push_back(sum / period);
		sum = 0;
	}

	return avg;
}

/*!
 * @brief calculates rate of change
 * @details rate of change calculation
 * @param input the stock prices in a vector
 * @param period the length of a period
 */
std::vector<float> GAnalysis::RateofChange(const std::vector<float>& input, unsigned int period)
{
	std::vector<float> roc;
	unsigned int count = 0;
	unsigned int length = input.size();
	for (unsigned int i = period - 1; i <= length - 1; ++i)
	{
		roc.push_back(((input[i] - input[count]) / input[count]) * 100);
		++count;
	}

	return roc;
}

/*!
 * @brief calculates exponential moving average
 * @details exponential moving average calculation
 * @param input the stock prices in a vector
 * @param period the length of a period
 */
std::vector<float> GAnalysis::ExpMovingAverage(const std::vector<float>& input, unsigned int period)
{
	std::vector<float> exp_avg, sma;
	float smooth;
	unsigned int counter = 0;
	sma = SimpleMovingAverage(input, period);
	smooth = 2.0 / (period + 1);
	unsigned int len = input.size();
	for (unsigned int i = period - 1; i <= len - 1; ++i)
	{
		if (counter == 0)
		{
			exp_avg.push_back(sma[counter]);
		}
		else
		{
			exp_avg.push_back(smooth * (input[period - 1 + counter] - exp_avg[counter - 1]) +
							  exp_avg[counter - 1]);
		}
		++counter;
	}

	return exp_avg;
}

/*!
 * @brief calculates Bollinger band
 * @details Bollinger band calculation
 * @param input the stock prices in a vector
 * @param period the length of a period
 * @param std_dev the number of std. deviations used
 */
std::pair<std::vector<float>, std::vector<float> >
GAnalysis::Bollinger(const std::vector<float>& input, unsigned int period, unsigned int std_dev)
{
	std::vector<float> range, lower_band, upper_band;
	std::vector<float> sma = SimpleMovingAverage(input, period);
	float sd, sum = 0, mean, intermediate = 0;
	unsigned int len = input.size();
	for (unsigned int i = 0; i <= len - period; ++i)
	{
		range.assign(input.begin() + i, input.begin() + i + period);

		for (unsigned int k = 0; k < range.size(); ++k)
		{
			sum = sum + range[k];
		}
		mean = sum / range.size();

		for (unsigned int m = 0; m < range.size(); ++m)
		{
			intermediate = intermediate + ((range[m] - mean) * (range[m] - mean));
		}

		sd = sqrt(intermediate / range.size());
		mean = 0;
		intermediate = 0;
		lower_band.push_back(sma[i] - (std_dev * sd));
		upper_band.push_back(sma[i] + (std_dev * sd));
	}
	std::pair<std::vector<float>, std::vector<float> > bollinger_band =
		make_pair(lower_band, upper_band);
	return bollinger_band;
}

/*!
 * @brief calculates gain loss
 * @details gain loss calculation
 * @param input the stock prices in a vector
 */
std::vector<float> GAnalysis::GainLoss(const std::vector<float>& input)
{
	std::vector<float> gain_loss;
	unsigned int length = input.size();
	for (unsigned int i = 1; i <= length - 1; ++i)
	{
		gain_loss.push_back(input[i] - input[i - 1]);
	}

	return gain_loss;
}

/*!
 * @brief calculates RSI
 * @details RSI calculation
 * @param input the stock prices in a vector
 * @param period the length of a period
 */
std::vector<float> GAnalysis::RSI(const std::vector<float>& input, unsigned int period)
{
	std::vector<float> rsi_value;
	std::vector<float> gain_loss = GainLoss(input);
	unsigned int len = input.size();
	float gain = 0, loss = 0, avg_gain = 0, avg_loss = 0, prev_avg_gain, prev_avg_loss, rs;

	for (unsigned int j = 0; j < period; ++j)
	{
		if (gain_loss[j] <= 0)
			loss = loss + gain_loss[j];
		else
			gain = gain + gain_loss[j];
	}

	for (unsigned int i = 0; i < len - period; ++i)
	{

		if (i == 0 && loss != 0)
		{
			avg_gain = gain / period;
			avg_loss = -1 * loss / period; // loss is a positive value
			rs = avg_gain / avg_loss;
			rsi_value.push_back(100 - (100 / (1 + rs)));
			prev_avg_gain = avg_gain;
			prev_avg_loss = avg_loss;
		}
		else if (gain_loss[i + period - 1] <= 0 &&
				 ((prev_avg_loss * (period - 1) - gain_loss[i + period - 1]) / period) != 0)
		{

			rs = (prev_avg_gain * (period - 1) / period) /
				 ((prev_avg_loss * (period - 1) - gain_loss[i + period - 1]) /
				  period); // -gainloss[i+period] since loss is negative here
			rsi_value.push_back(100 - (100 / (1 + rs)));
			prev_avg_gain = prev_avg_gain * (period - 1) / period;
			prev_avg_loss = (prev_avg_loss * (period - 1) - gain_loss[i + period - 1]) / period;
		}

		else if (gain_loss[i + period - 1] > 0 && (prev_avg_loss * (period - 1) / period) != 0)
		{
			rs = ((prev_avg_gain * (period - 1) + gain_loss[i + period - 1]) / period) /
				 (prev_avg_loss * (period - 1) / period);
			rsi_value.push_back(100 - (100 / (1 + rs)));
			prev_avg_gain = (prev_avg_gain * (period - 1) + gain_loss[i + period - 1]) / period;
			prev_avg_loss = prev_avg_loss * (period - 1) / period;
		}

		else
			rsi_value.push_back(100);
	}

	return rsi_value;
}

/*!
 * @brief calculates OBV
 * @details OBV calculation
 * @param input the closing stock prices in a vector
 * @param volume the corresponding volumes in a vector
 */
std::vector<float> GAnalysis::OBV(const std::vector<float>& close, const std::vector<float>& volume)
{
	unsigned int len = close.size();
	float prev_price = 0.0f;
	// float prev_vol = 0.0f;
	std::vector<float> obvs;

	for (unsigned int i = 0; i < len; ++i)
	{
		if (i == 0)
			obvs.push_back(abs(volume[i]));
		else if (close[i] < prev_price)
			obvs.push_back(obvs[i - 1] - volume[i]);
		else if (close[i] > prev_price)
			obvs.push_back(obvs[i - 1] + volume[i]);
		else
			obvs.push_back(obvs[i - 1]);

		prev_price = close[i];
		// prev_vol = volume[i];
	}

	return obvs;
}

/*!
 * @brief calculates vortex indicators
 * @details vortex indicators calculation
 * @param high the daily high stock prices in a vector
 * @param low the daily low stock prices in a vector
 * @param close the closing stock prices in a vector
 * @param period the length of a period
 */
std::pair<std::vector<float>, std::vector<float> >
GAnalysis::Vortex(const std::vector<float>& high, const std::vector<float>& low,
				  const std::vector<float>& close, unsigned int period)
{
	unsigned int len = high.size();
	int tr1, tr2, tr3, length, tr_sum, pos_wm_sum, neg_wm_sum;
	std::vector<float> pos_wm, neg_wm, tr, pos_vortex, neg_vortex;

	for (unsigned int i = 0; i < len - 1; ++i)
	{
		if (high[i + 1] - low[i] > 0)
			pos_wm.push_back(high[i + 1] - low[i]);
		else
			pos_wm.push_back(-1 * (high[i + 1] - low[i]));

		if (low[i + 1] - high[i] > 0)
			neg_wm.push_back(low[i + 1] - high[i]);
		else
			neg_wm.push_back(-1 * (low[i + 1] - high[i]));

		tr1 = high[i + 1] - low[i + 1];
		if (high[i + 1] - close[i] > 0)
			tr2 = high[i + 1] - close[i];
		else
			tr2 = -1 * (high[i + 1] - close[i]);

		if (low[i + 1] - close[i] > 0)
			tr3 = low[i + 1] - close[i];
		else
			tr3 = -1 * (low[i + 1] - close[i]);

		if ((tr1 > tr2) && (tr1 > tr3))
			tr.push_back(tr1);

		if ((tr2 > tr1) && (tr2 > tr3))
			tr.push_back(tr2);
		else
			tr.push_back(tr3);
	}
	for (unsigned int k = 0; k <= len - period; ++k)
	{
		for (unsigned int j = k; j < k + period; ++j)
		{
			tr_sum = tr_sum + tr[j];
			pos_wm_sum = pos_wm_sum + pos_wm[j];
			neg_wm_sum = neg_wm_sum + neg_wm[j];
		}
		pos_vortex.push_back(pos_wm_sum / tr_sum);
		neg_vortex.push_back(neg_wm_sum / tr_sum);

		tr_sum = 0;
		pos_wm_sum = 0;
		neg_wm_sum = 0;
	}
	std::pair<std::vector<float>, std::vector<float> > vortex_band(pos_vortex, neg_vortex);
	return vortex_band;
}

/*std::vector<float> GAnalysis::PSAR(const std::vector<float>&	open, const std::vector<float>&
high,
	const std::vector<float>& low, const std::vector<float>& close, const std::vector<float>&
volume)
	{
		for (unsigned int i = 0; i < open.size(); ++i)
		{
		}
	}
}*/

/*!
 * @brief calculates MFI
 * @details money flow index calculation
 * @param low the daily low stock prices in a vector
 * @param close the closing stock prices in a vector
 * @param volume the corresponding volume
 * @param period the length of a period
 */
std::vector<float> GAnalysis::MFI(const std::vector<float>& high, const std::vector<float>& low,
								  const std::vector<float>& close, const std::vector<float>& volume,
								  unsigned int period)
{
	float pos_money = 0, neg_money = 0;
	std::vector<float> money_flow;
	for (unsigned int i = 0; i < high.size() - period; ++i)
	{
		for (unsigned int j = i; j < i + period; ++j)
		{
			if ((high[j] + low[j] + close[j]) < (high[j + 1] + low[j + 1] + close[j + 1]))
				pos_money =
					pos_money + ((high[j + 1] + low[j + 1] + close[j + 1]) / 3 * volume[j + 1]);
			else
				neg_money =
					neg_money + ((high[j + 1] + low[j + 1] + close[j + 1]) / 3 * volume[j + 1]);
		}
		if (neg_money == 0)
		{
			money_flow.clear();
			return money_flow;
		}

		money_flow.push_back(100 - (100 / (1 + (pos_money / neg_money)))); // note this could have a
																		   // divide by 0 error if
																		   // there are no negative
																		   // money flows in the
																		   // entire period
		pos_money = 0;
		neg_money = 0;
	}

	return money_flow;
}

/*!
 * @brief calculates Stochastic oscilator
 * @details Stochastic oscilator calculation
 * @param high the daily high stock prices in a vector
 * @param low the daily low stock prices in a vector
 * @param close the closing stock prices in a vector
 * @param period the length of a period
 */
std::vector<float> GAnalysis::Stochastic(const std::vector<float>& high,
										 const std::vector<float>& low,
										 const std::vector<float>& close, unsigned int period)
{
	float max, min;
	std::vector<float> SO;
	for (unsigned int i = 0; i <= high.size() - period; ++i)
	{
		for (unsigned int j = i; j < i + period; ++j)
		{
			if (j == i)
			{
				max = high[j];
				min = low[j];
			}
			else
			{
				if (high[j] > max)
					max = high[j];
				if (low[j] < min)
					min = low[j];
			}
		}
		SO.push_back((close[i + period - 1] - min) / (max - min) * 100);
	}

	return SO;
}

/*!
 * @brief calculates VWAP
 * @details VWAP calculation
 * @param high the daily high stock prices in a vector
 * @param low the daily low stock prices in a vector
 * @param close the closing stock prices in a vector
 * @param volume the corresponding volume
 * @param period the length of a period
 */
std::vector<float> GAnalysis::VWAP(const std::vector<float>& high, const std::vector<float>& low,
								   const std::vector<float>& close,
								   const std::vector<float>& volume, unsigned int period)
{
	float vol = 0, vol_close = 0;
	std::vector<float> vwap;
	for (unsigned int i = 0; i < close.size() - period; ++i)
	{
		for (unsigned int j = i; j < i + period; ++j)
		{
			vol = vol + volume[j];
			vol_close = vol_close + (volume[j] * (high[j] + low[j] + close[j]) / 3);
		}
		vwap.push_back(vol_close / vol);
		vol = 0;
		vol_close = 0;
	}

	return vwap;
}

/*!
 * @brief calculates WilliamsR
 * @details WilliamsR calculation
 * @param high the daily high stock prices in a vector
 * @param low the daily low stock prices in a vector
 * @param close the closing stock prices in a vector
 * @param period the length of a period
 */
std::vector<float> GAnalysis::WilliamsR(const std::vector<float>& high,
										const std::vector<float>& low,
										const std::vector<float>& close, unsigned int period)
{
	std::vector<float> williams;
	float highest, lowest;
	for (unsigned int i = 0; i <= high.size() - period; ++i)
	{
		for (unsigned int j = 0; j < period; ++j)
		{
			if (j == 0)
			{
				highest = high[j + i];
				lowest = low[j + i];
			}
			else
			{
				if (high[j + i] > highest)
					highest = high[j + i];

				if (low[j + i] < lowest)
					lowest = low[j + i];
			}
		}
		williams.push_back(((highest - close[period - 1 + i]) / (highest - lowest)) * -100);
	}

	return williams;
}
