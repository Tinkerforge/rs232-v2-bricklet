function octave_example_loopback()
    more off;

    % For this example connect the RX1 and TX pin to receive the send message

    HOST = "localhost";
    PORT = 4223;
    UID = "XYZ"; % Change XYZ to the UID of your RS232 Bricklet 2.0

    ipcon = javaObject("com.tinkerforge.IPConnection"); % Create IP connection
    rs232 = javaObject("com.tinkerforge.BrickletRS232V2", UID, ipcon); % Create device object

    ipcon.connect(HOST, PORT); % Connect to brickd
    % Don't use device before ipcon is connected

    % Register read callback to function cb_read
    rs232.addReadCallback(@cb_read);

    % Enable read callback
    rs232.enableReadCallback();

    % Write "test" string
    rs232.write(string2chars("test")); % FIXME: throws java.lang.StringIndexOutOfBoundsException: String index out of range: 0

    input("Press key to exit\n", "s");
    ipcon.disconnect();
end

% Callback function for read callback
function cb_read(e)
    fprintf("Message: \"%s\"\n", chars2string(e.message));
end

% Convert string to array
function chars = string2chars(string)
    chars = javaArray("java.lang.String", length(string));

    for i = 1:length(string)
        chars(i) = substr(string, i, 1);
    end
end

% Assume that the message consists of ASCII characters and
% convert it from an array of chars to a string
function string = chars2string(chars)
    string = "";

    for i = 1:length(chars)
        string = strcat(string, chars(i));
    end
end
