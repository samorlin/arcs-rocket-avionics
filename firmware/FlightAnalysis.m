clear all  % Clears the data memory
close all  % Closes all old graph windows before running
clc        % Clears the command window text

% Read directly from the file in your Current Folder
% NEW HEADER: TimeMS,Altitude,Pressure,Temp,AccX,AccY,AccZ,GyroX,GyroY,GyroZ,BatteryV
Data = readmatrix("payload_log.csv");

% Separate data into sections based on your NEW column order
Time = Data(:, 1);        % recorded in ms
Altitude = Data(:, 2);    % recorded in m
Pressure = Data(:, 3);    % recorded in hPa (NEW)
Temperature = Data(:, 4); % recorded in Celsius (NEW)
AccX = Data(:, 5);        % acceleration X (m/s^2)
AccY = Data(:, 6);        % acceleration Y (m/s^2)
AccZ = Data(:, 7);        % acceleration Z (m/s^2)
GyroX = Data(:, 8);       % Gyro X (radians/s)
GyroY = Data(:, 9);       % Gyro Y (radians/s)
GyroZ = Data(:, 10);      % Gyro Z (radians/s)
BattVolt = Data(:, 11);   % Battery Voltage (V)

% Convert time to seconds for proper numerical calculations
Time_sec = Time / 1000;

% =========================================================================
% VELOCITY & DISTANCE CALCULATIONS (ALTIMETER ONLY)
% =========================================================================
% Smooth out the tiny sensor noise jumps using a moving average
Altitude_Smooth = smoothdata(Altitude, 'movmean', 20);

% Calculate velocity by finding the slope of the SMOOTHED altitude
vVertical = gradient(Altitude_Smooth, Time_sec); 
vMagnitude = abs(vVertical); % Speed magnitude (always positive)

% Integrate positive velocity to find total cumulative distance (odometer)
distance = cumtrapz(Time_sec, vMagnitude);

% Integrate raw vertical velocity (allowing negatives) to find Net Displacement
net_displacement = cumtrapz(Time_sec, vVertical);

% =========================================================================
% PLOTTING SECTION
% =========================================================================

% Figure 1: Altitude vs Time (Showing raw vs smoothed data)
F1 = figure(1);
plot(Time, Altitude, 'c:', 'LineWidth', 1) 
hold on;
plot(Time, Altitude_Smooth, 'b', 'LineWidth', 1.5) 
hold off;
xlabel('Time (ms)');
ylabel('Altitude (m)');
title('Altitude vs Time (Raw vs Smoothed)');
legend('Raw Altimeter Data', 'Smoothed Data', 'Location', 'best');
grid on

% Figure 2: Battery Voltage vs Time
F2 = figure(2);
plot(Time, BattVolt, 'r', 'LineWidth', 1.5)
xlabel('Time (ms)');
ylabel('Battery Voltage (V)');
title('Battery Voltage vs Time');
grid on 

% Figure 3: 3D Acceleration Graph (The "Gravity Sphere")
F3 = figure(3);
plot3(AccX, AccY, AccZ, 'g')
xlabel('Acceleration X (m/s^2)');
ylabel('Acceleration Y (m/s^2)');
zlabel('Acceleration Z (m/s^2)');
title('Raw 3D Acceleration Plot (Gravity Sphere)');
grid on
axis equal 

% Figure 4: Vertical Velocity vs Time 
F4 = figure(4);
plot(Time, vVertical, 'k', 'LineWidth', 1.5)
grid on
xlabel('Time (ms)')
ylabel('Vertical Velocity (m/s)')
title('Vertical Velocity vs Time')

% Figure 5: Velocity Magnitude vs Time
F5 = figure(5);
plot(Time, vMagnitude, 'm', 'LineWidth', 1.5);
xlabel('Time (ms)');
ylabel('Velocity Magnitude (m/s)');
title('Velocity Magnitude vs Time');
grid on;

% Figure 6: Total Vertical Distance Traveled (The "Odometer")
F6 = figure(6);
plot(Time, distance, 'c', 'LineWidth', 1.5);
xlabel('Time (ms)');
ylabel('Total Distance Traveled (m)');
title('Total Vertical Distance Traveled vs Time (Odometer)');
grid on

% Figure 7: Net Displacement vs Time (Mirrors Altitude)
F7 = figure(7);
plot(Time, net_displacement, 'g', 'LineWidth', 1.5);
xlabel('Time (ms)');
ylabel('Net Displacement (m)');
title('Net Displacement vs Time');
grid on;

% Figure 8: Battery Voltage against Altitude Graph
F8 = figure(8);
plot(Altitude_Smooth, BattVolt, 'm', 'LineWidth', 1.5);
xlabel('Altitude (m)');
ylabel('Battery Voltage (V)');
title('Battery Voltage vs Altitude');
grid on

% Figure 9: Raw Gyroscope Data Graph
F9 = figure(9);
plot(Time, GyroX, 'r', Time, GyroY, 'g', Time, GyroZ, 'b');
xlabel('Time (ms)');
ylabel('Angle (radians/s)');
title('Raw Gyroscope Data vs Time');
legend('GyroX', 'GyroY', 'GyroZ');
grid on;

% Figure 10: Smoothed Altitude and Velocity on same graph
F10 = figure(10);
yyaxis left 
plot(Time, Altitude_Smooth, 'b-', 'LineWidth', 1.5);
ylabel('Altitude (m)');
yyaxis right
plot(Time, vMagnitude, 'r-', 'LineWidth', 1.5);
ylabel('Velocity Magnitude (m/s)');
xlabel('Time (ms)')
title('Smoothed Altitude and Velocity vs Time')
legend('Smoothed Altitude', 'Velocity')
grid on

% Figure 11: Atmospheric Pressure vs Time (NEW)
F11 = figure(11);
plot(Time, Pressure, 'b', 'LineWidth', 1.5);
xlabel('Time (ms)');
ylabel('Pressure (hPa)');
title('Atmospheric Pressure vs Time');
grid on;

% Figure 12: Internal Temperature vs Time (NEW)
F12 = figure(12);
plot(Time, Temperature, 'r', 'LineWidth', 1.5);
xlabel('Time (ms)');
ylabel('Temperature (°C)');
title('Payload Internal Temperature vs Time');
grid on;