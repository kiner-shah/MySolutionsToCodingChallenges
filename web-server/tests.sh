#!/usr/bin/sh

check_first_line() {
	url="$1"
	first_line=$(curl --path-as-is -s -D - -o /dev/null "$url" | sed -n '1p')
	printf '%s -> %s\n' "$url" "$first_line"
}

# Valid paths - 200
check_first_line http://localhost:3000/
check_first_line http://localhost:3000/index.html
check_first_line http://localhost:3000/about.html
check_first_line http://localhost:3000/images/logo.png
check_first_line http://localhost:3000/blog/post1.html

# Non-existent files - 404
check_first_line http://localhost:3000/doesnotexist.html
check_first_line http://localhost:3000/missing/file.txt
check_first_line http://localhost:3000/images/nope.png

# Directory traversal paths - ?
check_first_line http://localhost:3000/../secret.txt
check_first_line http://localhost:3000/../../etc/passwd
check_first_line http://localhost:3000/../../../home/user/.ssh/id_rsa
check_first_line http://localhost:3000/images/../../private.txt

# Encoding traversal paths - ?
check_first_line "http://localhost:3000/%2e%2e/%2e%2e/etc/passwd"
check_first_line "http://localhost:3000/..%2F..%2Fetc/passwd"
check_first_line "http://localhost:3000/%2e%2e%2fsecret.txt"
check_first_line "http://localhost:3000/..%5C..%5Cwindows/system.ini"

# Normalization tests - should resolve safely
check_first_line http://localhost:3000/subdir/../index.html
check_first_line http://localhost:3000/subdir/./file.txt
check_first_line http://localhost:3000/././index.html

# Backslash traversal (Windows-style) - ?
check_first_line http://localhost:3000/..\..\secret.txt
check_first_line http://localhost:3000/images\..\..\private.txt

# Fake root escape attempts - ?
check_first_line http://localhost:3000/www/../../index.html
check_first_line http://localhost:3000/www/../../../etc/passwd
