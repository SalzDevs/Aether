import time


def calc_fuel(initial_fuel_level,burn_rate,time):
    remaining_fuel = initial_fuel_level - (burn_rate * time)
    return max(0.0,remaining_fuel)

init_fuel = 450;
burn_rate = 10;

def main():
    for t in range(0,50):
        fuel = calc_fuel(init_fuel,burn_rate,t)
        print(f"Initial fuel:{init_fuel} Current fuel:{fuel}")
        time.sleep(0.5) 


main()

