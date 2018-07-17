Imports System
Imports Tinkerforge

' For this example connect the RX pin to the TX pin on the same Bricklet

Module ExampleLoopback
    Const HOST As String = "localhost"
    Const PORT As Integer = 4223
    Const UID As String = "XYZ" ' Change XYZ to the UID of your RS232 Bricklet 2.0

    ' Callback subroutine for read callback
    Sub ReadCB(ByVal sender As BrickletRS232V2, ByVal message As Char())
        ' Assume that the message consists of ASCII characters and convert it
        ' from an array of chars to a string
        Console.WriteLine("Message: ""{0}""", new String(message))
    End Sub

    Sub Main()
        Dim ipcon As New IPConnection() ' Create IP connection
        Dim rs232 As New BrickletRS232V2(UID, ipcon) ' Create device object

        ipcon.Connect(HOST, PORT) ' Connect to brickd
        ' Don't use device before ipcon is connected

        ' Register read callback to subroutine ReadCB
        AddHandler rs232.ReadCallback, AddressOf ReadCB

        ' Enable read callback
        rs232.EnableReadCallback()

        ' Write "test" string
        rs232.Write("test".ToCharArray())

        Console.WriteLine("Press key to exit")
        Console.ReadLine()
        ipcon.Disconnect()
    End Sub
End Module
