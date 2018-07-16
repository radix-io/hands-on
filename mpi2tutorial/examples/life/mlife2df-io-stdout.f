C
C
C  (C) 2004 by University of Chicago.
C      See COPYRIGHT in top-level directory.
C

C stdout implementation of checkpoint (no restart) for MPI Life
C
C Data output in matrix order: spaces represent dead cells,
C '*'s represent live ones.
C


       subroutine MLIFEIO_Init(comm)
       implicit none
       include 'mpif.h'
       integer comm
       integer ierr
       integer mlifeio_comm
       common /mlifeio/ mlifeio_comm
       save /mlifeio/
       call mpi_comm_dup( comm, mlifeio_comm, ierr )
       return
       end
C
       subroutine MLIFEIO_Finalize()
       implicit none
       integer ierr
       include 'mpif.h'
       integer mlifeio_comm
       common /mlifeio/ mlifeio_comm
       save /mlifeio/
       
       if (mlifeio_comm .ne. MPI_COMM_NULL) then
          call mpi_comm_free( mlifeio_comm, ierr )
       endif
       end

       subroutine MLIFEIO_Checkpoint(prefix, matrix, lRows, lCols,        &
     &                               GRows, GCols, iter, info )
       implicit none
       include 'mpif.h'
       include 'mlife2df.h'
       character*(*) prefix
       character*10 rowPrefix
       integer lRows, lCols, GRows, GCols, iter, info
       integer matrix(0:MaxLRows-1,0:MaxLCols-1)
C
       integer ierr, rank, nprocs, r, i, d
       integer GFirstRow, GFirstCol
       integer mlifeio_comm
       common /mlifeio/ mlifeio_comm
       save /mlifeio/

       call mpi_comm_size( mlifeio_comm, nprocs, ierr )
       call mpi_comm_rank( mlifeio_comm, rank, ierr )

       call MLIFE_MeshDecomp(rank, nprocs, GRows, GCols,                  &
     &                d,d,d,d,                                            &
     &                LRows, LCols, GFirstRow, GFirstCol)

C   each proc. writes its part of the display, in rank order
       if (rank .eq. 0) then 
          print *, "[H[2J# Iteration ", iter
       endif
       

C     Slow but simple ...
      do r=0, nprocs-1
         if (rank .eq. r) then
C     print rank 0 data first 
            do i=1, LRows
               if (GFirstCol .eq. 1) then
                  write(rowPrefix,1) 1+(i-1+GFirstRow+1), GFirstCol
               else 
                  write(rowPrefix,1) 1+(i-1+GFirstRow+1), GFirstCol+5
               endif
 1                format("[",i3.3,";",i3.3,"H" )
               
               call MLIFEIO_Row_print(rowPrefix, matrix(i,1), MaxLRows,    &
     &              LCols, i+GFirstRow-1, GFirstCol .eq. 1)
            enddo
         endif
         call mpi_barrier(mlifeio_comm, ierr )
      enddo
      
      if (rank .eq. nprocs-1) then
         print 1, GRows+3,1
      endif

C give time to see the results 
      call MLIFEIO_msleep(250)
      
      end

      subroutine MLIFEIO_Row_print(rowPrefix, data, lda, cols, rownr,       &
     &                             labelrow )
      implicit none
      include 'mpif.h'
      character*10 rowPrefix
      integer ierr
      integer lda, data(lda,*), cols, rownr
      logical labelrow
      integer i
      character*160 line
      character*5   rowLabel

      if (cols .gt. 160) then
          print *, 'This test exceeds the maximum column count'
          call mpi_abort( MPI_COMM_WORLD, ierr )
      endif
      line = " "
      do i=1, cols
         if (data(1,i) .eq. 1) then
            line(i:i) = "*";
         endif
      enddo
      if (labelrow) then
          write( rowLabel, 1 ) rownr
 1        format( i3, ": " )
          print *, rowPrefix(1:len(rowPrefix)) // rowLabel //            &
     &             line(1:cols)
      else
          print *, rowPrefix(1:len(rowPrefix)) // line(1:cols)
       endif
      return
      end

      integer function MLIFEIO_Restart(prefix, matrix, GRows, GCols,     &
     &     iter, info)
      implicit none
      character*(*) prefix
      integer matrix(*), GRows, GCols, iter, info
      include 'mpif.h'
      MLIFEIO_Restart = MPI_ERR_IO
      return
      end
C
      integer function MLIFEIO_Can_restart()
      implicit none
      MLIFEIO_Can_restart = 0
      return 
      end
C
      subroutine MLIFEIO_msleep( msec )
      implicit none
      integer msec
      double precision t0
      include 'mpif.h'
C
      t0 = mpi_wtime()
      do while (mpi_wtime() - t0 < 0.001 * msec ) 
      enddo
      return
      end
