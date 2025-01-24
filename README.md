# Customer Loyalty Subsystem
Final codes [here]<br>
<center><img src="https://github.com/trinadiak/responsi/blob/main/Responsi_3_B/hardware.jpg" width="200" height="350"></center></br>
The Customer Loyalty Subsystem is a part of the _vending machine_ project designed to verify customer loyalty by validating voucher codes entered by the buyer. This system ensures that customers can use their vouchers correctly, enhancing interaction and the shopping experience with the vending machine. The subsystem employs two main microcontroller components: ESP32 and PSoC.

The mechanism of this subsystem involves several stages. First, customers are prompted to enter a 2-digit voucher code generated by the Feedback subsystem. Voucher code entry is performed by pressing push buttons. There are three push buttons with distinct functions: push button 1 to increase the value of the first digit, push button 2 to increase the value of the second digit, and the Enter button to finalize the voucher code. Users are given three attempts to correctly input the voucher code. If they fail more than three times, the voucher will be voided and become invalid.

After the voucher code is entered, the PSoC microcontroller sends the user-entered voucher code data to the ESP32 microcontroller using the Modbus communication protocol. The ESP32 microcontroller is responsible for sending the `user_voucherEligible` and `user_voucherChange` codes to the server via an HTTP connection to verify the voucher. The `user_voucherEligible` code has two possible values: `0` if the user is not eligible for the voucher and `1` if the user is eligible. The `user_voucherChange` code has three possible values: `None` if the voucher remains unchanged, `Used` if the voucher has already been used, and a new voucher code. 

The voucher code is checked against the database on the server to ensure its validity. The server verifies whether the voucher code matches one stored in the system, whether the voucher is still valid, and whether it has not yet been used. 

If the voucher code is valid, the server sends back information to the Customer Loyalty subsystem via the ESP32, including `user_voucher`, which contains information about the voucher available for the user, and `user_history`, which contains information about the user's purchase history. The ESP32 then signals the PSoC to activate the discount or reward associated with the voucher. A green LED will light up to indicate that the voucher code is valid. Conversely, if the voucher code is invalid, a red LED will light up to indicate that the voucher code is not valid.
