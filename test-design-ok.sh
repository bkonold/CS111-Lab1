#! /bin/sh

# UCLA CS 111 Lab 1 - Test that valid syntax is processed correctly.

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test.sh <<'EOF'
true

g++ -c foo.c

: : :

cat < /etc/passwd | tr a-z A-Z | sort -u || echo sort failed!

a b<c > d

cat < /etc/passwd | tr a-z A-Z | sort -u > out || echo sort failed!

a&&b||
 c &&
  d | e && f|

g<h

cat >& /etc/passwd | tr a-z A-Z | sort -u >| out || echo sort failed!

a<&b>&c|d<&e>>f|g<&h>|i

2<> /etc/passwd

(a && b) <& c >> d

(a || b) >& d

(a >> b) >> d

(a || b) >| d

(a >| b) && (c >& d)

# This is a weird example: nobody would ever want to run this.
a<b>c|d<e>f|g<h>i
EOF

cat >test.exp <<'EOF'
# 1
  true
# 2
  g++ -c foo.c
# 3
  : : :
# 4
      cat</etc/passwd \
    |
      tr a-z A-Z \
    |
      sort -u \
  ||
    echo sort failed!
# 5
  a b<c>d
# 6
      cat</etc/passwd \
    |
      tr a-z A-Z \
    |
      sort -u>out \
  ||
    echo sort failed!
# 7
        a \
      &&
        b \
    ||
      c \
  &&
      d \
    |
      e \
  &&
      f \
    |
      g<h
# 8
      cat>&/etc/passwd \
    |
      tr a-z A-Z \
    |
      sort -u>|out \
  ||
    echo sort failed!
# 9
    a<&b>&c \
  |
    d<&e>>f \
  |
    g<&h>|i
# 10
  2<>/etc/passwd
# 11
  (
     a \
   &&
     b
  )<&c>>d
# 12
  (
     a \
   ||
     b
  )>&d
# 13
  (
   a>>b
  )>>d
# 14
  (
     a \
   ||
     b
  )>|d
# 15
    (
     a>|b
    ) \
  &&
    (
     c>&d
    )
# 16
    a<b>c \
  |
    d<e>f \
  |
    g<h>i
EOF

../timetrash -p test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"
