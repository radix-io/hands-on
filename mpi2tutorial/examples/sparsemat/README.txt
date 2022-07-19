The Compressed-Spare-Row (CSR) format is defined by 3 arrays and one
integer:
n       - number of rows in the matrix (integer)
ia[n+1] - index of data for each row.  Last (n+1st) element gives the 
          total length of the arrays a and ja (integer)
a[nz]   - values of matrix entries (double)
ja[nz]  - column number for corresponding a entry

All index values are 1-origin (this format was developed for Fortran)

For example, the sparse matrix
     ( 1  2  0
       3  0  0
       0  4  5 )
is stored as:
n = 3
ia = ( 1, 3, 4, 6 )
a  = ( 1, 2, 3, 4, 5 )
ja = ( 1, 2, 1, 2, 3 )

The number of elements on the ith row is ia[i+1] - ia[i] (which is why ia
has n+1, not just n entries).
