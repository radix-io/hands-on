C 
C THIS EXAMPLE IS NOT COMPLETE
C
      program mlife2df
      implicit none
      include 'mpif.h'
      include 'mlife2df.h'
      integer rank, ierr
      double precision time, life
C
      call mpi_init(ierr)
C
C Set and/or update the default problem parameters
C      
      call mlife_get_args()

      call mlifeio_init( MPI_COMM_WORLD )
      
      time = life( opt_rows, opt_cols, opt_iter, MPI_COMM_WORLD )
      
      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
      if (rank .eq. 0) then
         print *, '[', rank, '] Life finished in ', time,
     $        ' secs of calculation' 
      endif

      call mlifeio_finalize()
      call mpi_finalize(ierr)
      end
C
      double precision function life( rows, cols, ntimes, comm )
      implicit none
      include 'mpif.h'
      include 'mlife2df.h'
      integer rows, cols, ntimes, comm
      integer rank, nprocs, ierr
      integer next, prev, left, right, i, j, k
      integer LRows, LCols, GFirstRow, GFirstCol
      integer mlife_nextstate
      external rand
      real  rand
      integer matrix(MaxLRows,MaxLCols), matrix2(MaxLRows,MaxLCols)
      double precision slavetime, totaltime, starttime

      call mpi_comm_size( comm, nprocs, ierr )
      call mpi_comm_rank( comm, rank, ierr )

      call mlife_meshdecomp( rank, nprocs, rows, cols, left, right, prev
     $     , next, LRows, Lcols, GFirstRow, GFirstCol )

C Initialize boundaries
      do j=1, LCols+2
         matrix(1,j)        = DIES
         matrix(LRows+2,j)  = DIES
         matrix2(1,j)       = DIES
         matrix2(LRows+2,j) = DIES
      enddo
      do i=1, LRows+2
         matrix(i,1)        = DIES
         matrix(i,LCols+2)  = DIES
         matrix2(i,1)       = DIES
         matrix2(i,LCols+2) = DIES
      enddo
      
C Initialize the matrix      
      do i=1, LRows
         call srand(1000 + (i + GFirstRow - 1))
         do j=1, LCols
            if (rand(0) .gt. 0.5) then 
               matrix(i,j) = BORN
            else
               matrix(i,j) = DIES
            endif
         enddo
      enddo

C     call MLIFE2D_InitBlinker( matrix, LRows, LCols )
      call MLIFE2D_InitEdge( matrix, LRows, LCols )

      call mlife_exchange_init(comm, matrix, matrix2,                       &
     &                         rows, cols, LRows, LCols,                    &
     &                         prev, next, left, right )

      call mlifeio_checkpoint( opt_prefix, matrix, LRows, LCols,            &
     &                            rows, cols, 0, MPI_INFO_NULL )
      starttime = MPI_Wtime()
      do k=1, ntimes, 2
         call mlife_exchange( matrix, LRows, LCols, 1 )

         do i=2, LRows+1
            do j=2, LCols+1
               matrix2(i,j) = mlife_nextstate(matrix,i,j)
            enddo
         enddo

         call mlifeio_checkpoint( opt_prefix, matrix2, LRows, LCols,      &
     &                            rows, cols, k, MPI_INFO_NULL )

         call mlife_exchange( matrix2, LRows, LCols, 2 )

         do i=2, LRows+1
            do j=2, LCols+1
               matrix(i,j) = mlife_nextstate(matrix2,i,j)
            enddo
         enddo

         call mlifeio_checkpoint( opt_prefix, matrix, LRows, LCols,       &
     &                            rows, cols, k+1, MPI_INFO_NULL )

      enddo

      slavetime = MPI_Wtime() - starttime
      call mpi_reduce( slavetime, totaltime, 1, MPI_DOUBLE_PRECISION,
     $     MPI_SUM, 0, comm, ierr )

      call mlife_exchange_finalize

      life = totaltime
      end
c
      subroutine mlife_meshdecomp( rank, nprocs, GRows, GCols, left,
     $     right, top, bottom, LRows, LCols, GFirstRow, GFirstCol )
      implicit none
      include 'mpif.h'
      integer rank, nprocs, GRows, GCols, left, right, top, bottom,
     $     LRows, LCols, GFirstRow, GFirstCol
      integer dims(2), ierr, npcols, prow, pcol, firstrow, firstcol
      integer lastrow, lastcol
      include 'mlife2df.h'
      
      dims(1) = opt_prows
      dims(2) = opt_pcols

      call mpi_dims_create( nprocs, 2, dims, ierr )
      
      npcols = dims(2)

      prow = rank / dims(2)
      pcol = mod(rank, dims(2))

      print *, dims
      print *, prow, pcol

C Compute the neighbors
      left   = MPI_PROC_NULL
      right  = MPI_PROC_NULL
      top    = MPI_PROC_NULL
      bottom = MPI_PROC_NULL
      if (prow .gt. 0) top = rank - dims(2)
      if (pcol .gt. 0) left = rank - 1
      if (prow .lt. dims(1) - 1) bottom = rank + dims(2)
      if (pcol .lt. dims(2) - 1) right = rank + 1

      print *, top, left, bottom, right

C Compute the decomposition of the global mesh
      firstcol = 1 + pcol * (GCols / dims(2))
      firstrow = 1 + prow * (GRows / dims(1))
      if (pcol .eq. dims(2) - 1) then
         lastcol = GCols
      else
         lastcol = 1 + (pcol + 1) * (GCols / dims(2)) - 1
      endif
      if (prow .eq. dims(1) - 1) then
         lastrow = GRows
      else
         lastrow = 1 + (prow + 1) * (GRows / dims(1)) - 1
      endif

      LRows     = lastrow - firstrow + 1
      LCols     = lastcol - firstcol + 1
      GFirstRow = firstrow
      GFirstCol = firstcol

      end

      integer function mlife_nextstate( matrix, row, col )
      implicit none
      include 'mlife2df.h'
      integer matrix(MaxLRows,MaxLCols), row, col
      integer sum, result

      sum = matrix(row-1,col-1) + matrix(row-1,col) + matrix(row-1,col+1
     $     ) + matrix(row,col-1) + matrix(row,col+1) + matrix(row+1,col
     $     -1) + matrix(row+1,col) + matrix(row+1,col+1)
      result = matrix(row,col)
      if (sum .lt. 2 .or. sum .gt. 3) then
         result = DIES
      elseif (sum .eq. 3) then
         result = BORN
      endif
      
      mlife_nextstate = result
      end
C
      subroutine mlife_get_args
      implicit none
      include 'mlife2df.h'
C
      opt_rows = 25
      opt_cols = 70
      opt_iter = 10
      opt_prows = 0
      opt_pcols = 0
      opt_restart_iter = -1
      opt_prefix = "mlife"
C
      end
C
      subroutine MLIFE2D_InitBlinker( matrix, LRows, LCols )
      implicit none
      include 'mlife2df.h'
      integer matrix(0:MaxLRows-1,0:MaxLCols-1), LRows, LCols
      integer i,j
C
      do i=0,LRows+1
         do j=0,LCols+1
            matrix(i,j) = DIES
         enddo
      enddo
      matrix(2,2) = BORN
      matrix(3,2) = BORN
      matrix(4,2) = BORN

      end
C
      subroutine MLIFE2D_InitEdge( matrix, LRows, LCols )
C Create a blinker that spans an edge between processes
      implicit none
      include 'mlife2df.h'
      integer matrix(0:MaxLRows-1,0:MaxLCols-1), LRows, LCols
      integer i,j
C
      do i=0,LRows+1
         do j=0,LCols+1
            matrix(i,j) = DIES
         enddo
      enddo
      matrix(LRows-2,LCols) = BORN
      matrix(LRows-1,LCols) = BORN
      matrix(LRows-0,LCols) = BORN

      end
