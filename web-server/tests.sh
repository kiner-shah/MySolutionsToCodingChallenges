#!/usr/bin/sh

cleanup_static_escape_test() {
	rm -f www/prefix_escape
	rm -rf www2
}

cleanup_static_escape_test
trap cleanup_static_escape_test EXIT

mkdir -p www2
printf 'should not be served\n' > www2/secret.txt
ln -s ../www2 www/prefix_escape

check_first_line() {
	url="$1"
	first_line=$(curl --path-as-is -s -D - -o /dev/null "$url" | sed -n '1p')
	printf '%s -> %s\n' "$url" "$first_line"
}

# Valid paths - 200
check_first_line http://localhost:3000/
check_first_line http://localhost:3000/index.html
check_first_line http://localhost:3000/version
check_first_line http://localhost:3000/my_pic.png

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

# Symlink escape with shared root prefix - should not be served
check_first_line http://localhost:3000/prefix_escape/secret.txt
