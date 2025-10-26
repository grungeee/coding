import os
from bs4 import BeautifulSoup

# Directory where your message HTML files are located
directory = "~/Downloads/Telegram Desktop/Jana_and_Me_export"

# Function to extract and format messages from an HTML file
def extract_messages_from_file(file_path):
    with open(file_path, 'r', encoding='utf-8') as file:
        html_content = file.read()

    soup = BeautifulSoup(html_content, 'html.parser')
    messages = soup.find_all('div', class_='message')

    formatted_messages = []
    for message in messages:
        # Extract the date from the message
        date_details = message.find('div', class_='pull_right date details')
        from_name = message.find('div', class_='from_name')
        text = message.find('div', class_='text')

        if date_details and from_name and text:
            date = date_details['title'].split()[0]  # Extract date in DD.MM.YYYY format
            sender = from_name.get_text(strip=True)
            message_text = text.get_text(strip=True)

            formatted_messages.append((date, sender, message_text))
    
    return formatted_messages

# Function to print out messages grouped by date
def print_messages_grouped_by_date(directory):
    all_messages = []

    # Iterate through all HTML files in the directory
    for file_name in sorted(os.listdir(directory)):
        if file_name.endswith(".html"):
            file_path = os.path.join(directory, file_name)
            all_messages.extend(extract_messages_from_file(file_path))

    # Group and print messages by date
    current_date = None
    for date, sender, message_text in all_messages:
        if date != current_date:
            print(f"\n[{date}]")
            current_date = date
        print(f"{sender}: {message_text}")

# Run the function
print_messages_grouped_by_date(directory)
