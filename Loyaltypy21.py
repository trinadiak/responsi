from quart import Quart, jsonify, request
import aiofiles
import asyncio
import time
from datetime import datetime, timedelta
import csv
from io import StringIO

app = Quart(__name__)

csv_filename = 'okedeh (1).csv'

# Function to read CSV values
async def read_values_from_csv():
    user_voucherEligible = 0
    user_voucherChange = 0
    user_voucher = 0

    async with aiofiles.open(csv_filename, mode='r') as file:
        rows = await file.readlines()
        csv_reader = csv.reader(rows)

        for i, row in enumerate(csv_reader):
            if i == 0:
                continue  # Skip header
            login_status = row[6].strip()
            if login_status == '1':  # Match login status
                user_voucherEligible = 1 if row[7].strip().lower() == 'yes' else 0
                user_voucherChange = int(row[8].strip()) if row[8].strip().isdigit() else 0
                user_voucher = int(row[5].strip()) if row[5].strip().isdigit() else 0
                break  # Process only the first matching row
    # Print the values for verification in cmd
    print("=== CSV Values ===")
    print(f"user_voucherEligible: {user_voucherEligible}")
    print(f"user_voucherChange: {user_voucherChange}")
    print(f"user_voucher: {user_voucher}")
    print("===================")

    return user_voucherEligible, user_voucherChange, user_voucher

# Route to get voucher data
@app.route('/get_data', methods=['GET'])
async def get_data():
    user_voucherEligible, user_voucherChange, user_voucher = await read_values_from_csv()
    voucher_code = user_voucher if user_voucherChange == 1 and user_voucherEligible == 1 else 0
    return jsonify({'user_voucher': voucher_code})
   

# Route to update voucher data
@app.route('/update_data', methods=['POST'])
async def update_data():
    try:
        data = await request.get_json()
        status = data.get('status', 0)
        new_voucherChange = data.get('user_voucherChange', 0)
        user_voucherEligible = data.get('user_voucherEligible', 0)

        async with aiofiles.open(csv_filename, mode='r') as file:
            rows = await file.readlines()

        csv_reader = csv.reader(rows)
        updated_rows = []
        for i, row in enumerate(csv_reader):
            if i == 0:
                updated_rows.append(row)  # Add header
            else:
                login_status = row[6].strip()
                if login_status == '1':  # Only update rows with login_status == '1'
                    row[7] = str(user_voucherEligible)  # Update eligibility
                    row[8] = str(new_voucherChange)  # Update change
                updated_rows.append(row)

        # Write updated rows back to the CSV
        async with aiofiles.open(csv_filename, mode='w') as file:
            for row in updated_rows:
                await file.write(','.join(row) + '\n')

        return jsonify({'status': 'Data updated successfully'}), 200

    except Exception as e:
        return jsonify({'status': 'Failed to update data', 'error': str(e)}), 500

mismatch_count = 0  # Initialize mismatch count

# Update Status
@app.route('/update_status', methods=['POST'])
async def update_status():
    global mismatch_count
    try:
        # Receive the Modbus data and HTTP server data
        data = await request.get_json()
        modbus_value = data.get('modbus_value')  # Modbus value
        server_value = data.get('server_value')  # HTTP server value

        # Check if values match
        if modbus_value != server_value:
            mismatch_count += 1
            print(f"Mismatch count: {mismatch_count}")
            
            # If mismatches reach 3, update user_voucherChange to 2
            if mismatch_count == 3:
                async with aiofiles.open(csv_filename, mode='r') as file:
                    rows = await file.readlines()

                csv_reader = csv.reader(rows)
                updated_rows = []
                for i, row in enumerate(csv_reader):
                    if i == 0:
                        updated_rows.append(row)  # Add header
                    else:
                        login_status = row[6].strip()
                        if login_status == '1':  # Only update rows with login_status == '1'
                            row[8] = '2'  # Update user_voucherChange to 2
                        updated_rows.append(row)

                # Write updated rows back to the CSV
                async with aiofiles.open(csv_filename, mode='w') as file:
                    for row in updated_rows:
                        await file.write(','.join(row) + '\n')

                print("Updated user_voucherChange to 2 due to 3 mismatches.")
                mismatch_count = 0  # Reset the count after updating
        else:
            mismatch_count = 0  # Reset count if values match

        return jsonify({'status': 'Status updated successfully'}), 200

    except Exception as e:
        return jsonify({'status': 'Failed to update status', 'error': str(e)}), 500

#Timer 5 minutes
async def timer_check():
    try:
        current_time = datetime.now()
        async with aiofiles.open(csv_filename, mode='r') as file:
            rows = await file.readlines()

        csv_reader = csv.reader(rows)
        updated_rows = []

        for i, row in enumerate(csv_reader):
            if i == 0:
                updated_rows.append(row)  # Keep header row
                continue

            # Ensure row has at least 27 columns
            if len(row) < 27:
                updated_rows.append(row)
                continue

            # Handle extra columns if any
            row = row[:27]

            # Extract necessary fields
            login_status = row[6].strip()
            timestamp = row[25].strip()

            if login_status == '1':
                if timestamp:
                    try:
                        current_date = datetime.now().date()
                        time_part = datetime.strptime(timestamp, '%H:%M:%S').time()
                        row_time = datetime.combine(current_date, time_part)
                        current_time = datetime.now()
                        time_diff = (current_time - row_time).total_seconds() / 60  # Difference in minutes

                        # Debug: Show time difference
                        print(current_time, row_time)
                        print(f"Row {i}: Timestamp found - {timestamp}, Time difference: {time_diff:.2f} minutes.")

                        if time_diff > 5:  # 5-minute timer
                            row[8] = '2'  # Update user_voucherChange
                            print(f"Row {i}: Timer exceeded 5 minutes. Updating user_voucherChange to 2.")
                    except ValueError:
                        print(f"Row {i}: Invalid timestamp format - {timestamp}.")
                else:
                    print(f"Row {i}: No timestamp detected. Waiting for timestamp input.")
            else:
                print(f"Row {i}: login_status is not '1'. Skipping row.")

            updated_rows.append(row)

        # Write updated rows back to the CSV file
        async with aiofiles.open(csv_filename, mode='w') as file:
            for row in updated_rows:
                await file.write(','.join(row) + '\n')

    except Exception as e:
        print(f"Error in timer check: {e}")



# Schedule the timer function
async def start_timer():
    while True:
        await timer_check()
        await asyncio.sleep(5)  # Run every minute

# Run the timer in the background
@app.before_serving
async def setup_timer():
    asyncio.create_task(start_timer())

# Run the app
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)

