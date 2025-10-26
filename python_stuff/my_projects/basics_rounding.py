
gyX = 14
leds_count = 12
segment_size = 360 / leds_count # 30
segment_shift = segment_size / 2


LEDindex = gyX / segment_size
mod_estLEDindex = gyX % segment_size
print( mod_estLEDindex ) 

print(LEDindex)


