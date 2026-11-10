#!/bin/bash
cat > well_3x3.txt <<'EOF'
4 1 2
1 3 0
2 0 5
EOF

cat > pivot_2x2.txt <<'EOF'
0 1
1 0
EOF

cat > singular_2x2.txt <<'EOF'
1 4
-1 -4
EOF

cat > tiny_2x2.txt <<'EOF'
1e-20 2e-20
3e-20 5e-20
EOF

cat > huge_2x2.txt <<'EOF'
1e20 2e20
3e20 5e20
EOF

cat > tiny_identity_3x3.txt <<'EOF'
1e-20 0 0
0 1e-20 0
0 0 1e-20
EOF

cat > huge_identity_3x3.txt <<'EOF'
1e20 0 0
0 1e20 0
0 0 1e20
EOF

cat > mixed_diag_3x3.txt <<'EOF'
1 0 0
0 1e-5 0
0 0 1e5
EOF

cat > identity_4x4.txt <<'EOF'
1 0 0 0
0 1 0 0
0 0 1 0
0 0 0 1
EOF

cat > triangular_4x4.txt <<'EOF'
1 0 0 0
2 3 0 0
4 5 6 0
7 8 9 10
EOF

cat > hilbert_3x3.txt <<'EOF'
1 0.5 0.3333333333333333
0.5 0.3333333333333333 0.25
0.3333333333333333 0.25 0.2
EOF

cat > hilbert_4x4.txt <<'EOF'
1 0.5 0.3333333333333333 0.25
0.5 0.3333333333333333 0.25 0.2
0.3333333333333333 0.25 0.2 0.1666666666666667
0.25 0.2 0.1666666666666667 0.1428571428571429
EOF

echo "Сгенерировано 12 тестовых файлов"