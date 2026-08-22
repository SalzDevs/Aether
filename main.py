import time

def calc_fuel(initial_fuel_level, burn_rate, time_elapsed):
    remaining_fuel = initial_fuel_level - (burn_rate * time_elapsed)
    return max(0.0, remaining_fuel)

def calc_engine_temp(current_temp, ambient_temp, heat_generated, engine_mass, specific_heat, cooling_rate, time_step):
    heat_loss = -cooling_rate * (current_temp - ambient_temp)
    heat_gain = heat_generated / (engine_mass * specific_heat)
    dT_dt = heat_loss + heat_gain

    next_temp = current_temp + (dT_dt * time_step)
    return next_temp

def main():
    init_fuel = 450
    burn_rate = 10

    engine_temp = 25.0        
    ambient_temp = 25.0       
    heat_generated = 1500.0   
    engine_mass = 120.0      
    specific_heat = 450.0     
    cooling_rate = 0.02       
    time_step = 1.0           

    print("Starting simulation...\n")

    for t in range(0, 50):
        fuel = calc_fuel(init_fuel, burn_rate, t)
        
        engine_temp = calc_engine_temp(
            engine_temp, 
            ambient_temp, 
            heat_generated, 
            engine_mass, 
            specific_heat, 
            cooling_rate, 
            time_step
        )
        
        print(f"Time: {t:02d}s | Fuel: {fuel:<3} | Engine Temp: {engine_temp:.2f}°C")
        time.sleep(0.5) 

main()

