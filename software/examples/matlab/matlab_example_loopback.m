function matlab_example_loopback()
    import com.tinkerforge.IPConnection;
    import com.tinkerforge.BrickletRS232V2;
    import java.lang.String;

    % For this example connect the RX1 and TX pin to receive the send message

    HOST = 'localhost';
    PORT = 4223;
    UID = 'XYZ'; % Change XYZ to the UID of your RS232 Bricklet 2.0

    ipcon = IPConnection(); % Create IP connection
    rs232 = handle(BrickletRS232V2(UID, ipcon), 'CallbackProperties'); % Create device object

    ipcon.connect(HOST, PORT); % Connect to brickd
    % Don't use device before ipcon is connected

    % Register read callback to function cb_read
    set(rs232, 'ReadCallback', @(h, e) cb_read(e));

    % Enable read callback
    rs232.enableReadCallback();

    % Write "test" string
    rs232.write(String('test').toCharArray());

    input('Press key to exit\n', 's');
    ipcon.disconnect();
end

% Callback function for read callback
function cb_read(e)
    fprintf('Message: %s\n', e.message);
end
