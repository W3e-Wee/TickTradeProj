"""Small independent reference calculation for the optional stretch check."""
import csv
from decimal import Decimal


def summarize(path, short_window, long_window):
    prices, quantities, timestamps = [], [], []
    numerator = Decimal(0)
    denominator = Decimal(0)
    with open(path, newline="", encoding="ascii") as stream:
        for row in csv.reader(stream):
            if not row or not row[0].isdigit():
                continue
            price, quantity, timestamp = Decimal(row[1]), Decimal(row[2]), int(row[4])
            prices.append(float(price))
            quantities.append(float(quantity))
            timestamps.append(timestamp)
            numerator += price * quantity
            denominator += quantity
    bars = 0
    previous_second = None
    for stamp in timestamps:
        second = stamp // 1_000_000
        if second != previous_second:
            bars += 1
            previous_second = second
    buys = sells = 0
    previous = 0
    ready = False
    for index in range(long_window - 1, len(prices)):
        short_avg = sum(prices[index - short_window + 1:index + 1]) / short_window
        long_avg = sum(prices[index - long_window + 1:index + 1]) / long_window
        difference = short_avg - long_avg
        epsilon = 0.5 / (100_000_000 * short_window * long_window)
        sign = 0 if abs(difference) < epsilon else (difference > 0) - (difference < 0)
        if ready and previous <= 0 and sign > 0:
            buys += 1
        elif ready and previous >= 0 and sign < 0:
            sells += 1
        previous = sign
        ready = True
    return {"trades": len(prices), "bars": bars, "buys": buys,
            "sells": sells, "vwap": float(numerator / denominator) if denominator else 0.0}
