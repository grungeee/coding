import math
import time

def calc_led_index(gyX):
    
    leds_count = 12
    segment_size = 360 / leds_count # 30
    segment_shift = segment_size / 2

    LEDindex = gyX / segment_size
    print(LEDindex)
    mod_estLEDindex = gyX % segment_size
    print(f"mod {mod_estLEDindex}")

    if LEDindex > 1:
        LEDindex = int(math.floor(LEDindex))
        if (mod_estLEDindex > segment_shift): # 40 % 30 = 10 > 15 /// 46 % 30 = 16 < 15 =>
            LEDindex += 1 
    else:
        if (mod_estLEDindex > segment_shift): # range <= 15
            LEDindex = 1
        else:
            LEDindex = 0
    # else:
        # LEDindex = LEDindex
    print(f"#3: {LEDindex}")
    return LEDindex
        
        
# for i in range(0, 360 - 1):
#     print(f"{i} : {math.floor(i / 30)}")


# while True:
# LEDindex = calc_led_index(int((input("gyX: "))))

visualLEDs = ["_", "_", "_", "_", "_", "_", "_", "_", "_", "_", "_"]
curLED = "X"
for i in range(0, 360, 15):  
    print(f" ====< {i} >====")
    LEDindex = calc_led_index(i)
    print(f"LEDindex: {LEDindex}")
    print(f"type: {type(LEDindex)}")
    visualLEDs[LEDindex] = curLED
    print(visualLEDs)
    time.sleep(0.5)
    print(" ====< END >====")




