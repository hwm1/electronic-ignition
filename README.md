'C' language code for an electronic ignition for a typical 4-stroke motorcycle or similar engine, 
written for the TM4C123G evaluation board.  Engine crankshaft has a magnet embedded in the 
flywheel, and a fixed two pole magnetic pickup with poles pickup_degrees apart combined with a 1/100 ms
timer is used to detect the speed of the engine to calculate the correct advance and the time to
fire the ignition.  Advance ranges from min_adv to max_adv degrees linerally, beginning at 1000 RPM 
and ending at 5000 RPM. TDC for the engine is 180 degrees from the second pickup pole.  Ignition 
coil firing circuit is connected to PF1, and pickup is connected to PF0.  For production code, values
should be suitably adjusted for the engine and a suitable watchdog timer should be added.  
