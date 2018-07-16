C SLIDE: 2D Life Code Walkthrough 
C
C  (C) 2004 by University of Chicago.
C      See COPYRIGHT in top-level directory.
C

       subroutine MLIFE_exchange_init(comm, matrix, matrix2,               &
     &                                rows, cols, LRows, LCols,            &
     &                                above, below, left, right )
       implicit none
       include 'mlife2df.h'
       include 'mpif.h'
       integer comm
       integer rows, cols, LRows, LCols
       integer matrix(0:MaxLRows-1,0:LCols+1)
       integer matrix2(0:MaxLRows-1,0:LCols+1)
       integer above, below, left, right
       integer ierr
C
       call mpi_comm_dup( comm, exch_comm, ierr )
       exch_above = above
       exch_below = below
       exch_left  = left
       exch_right = right
       end
C
       subroutine MLIFE_exchange_finalize()
       implicit none
       include 'mlife2df.h'
       include 'mpif.h'
       integer ierr
       call mpi_comm_free( exch_comm, ierr )
       end
C
       subroutine MLIFE_exchange(matrix, LRows, LCols, parity)
       implicit none
       integer parity
       include 'mpif.h'
       include 'mlife2df.h'
       integer LRows, LCols, matrix(0:MaxLRows-1,0:LCols+1)
       integer ierr, reqs(4), vectype
       save vectype
       data vectype/MPI_DATATYPE_NULL/
C
C     Send and receive boundary information */

      if (vectype .eq. MPI_DATATYPE_NULL) then
          call mpi_type_vector(LCols, 1, MaxLRows, MPI_INTEGER, vectype,     &
     &                         ierr)
          call mpi_type_commit(vectype, ierr)
      endif

C  first, move the top, bottom edges 
      call mpi_isend(matrix(1,1), 1, vectype,                               &
     &	      exch_above, 0, exch_comm, reqs(1), ierr )
      call mpi_irecv(matrix(0,1), 1, vectype,                               &
     &        exch_above, 0, exch_comm, reqs(2), ierr )
      call mpi_isend(matrix(LRows,1), 1, vectype,                           &
     &        exch_below, 0, exch_comm, reqs(3), ierr )
      call mpi_irecv(matrix(LRows+1,1), 1, vectype,                         &
     &	      exch_below, 0, exch_comm, reqs(4), ierr )
C We need to wait on these for the trick that we use to move
C the diagonal terms to work 
      call mpi_waitall( 4, reqs, MPI_STATUSES_IGNORE, ierr )
C
C move the left, right edges (including diagonals)
      call mpi_isend(matrix(0,1), LRows+2, MPI_INTEGER,                     &
     &	      exch_left, 0, exch_comm, reqs(1), ierr )
      call mpi_irecv(matrix(0,0), LRows+2, MPI_INTEGER,                     &
     &	      exch_left, 0, exch_comm, reqs(2), ierr )
      call mpi_isend(matrix(0,LCols), LRows+2, MPI_INTEGER,                 &
     &	      exch_right, 0, exch_comm, reqs(3), ierr )
      call mpi_irecv(matrix(0,LCols+1), LRows+2, MPI_INTEGER,               &
     &	      exch_right, 0, exch_comm, reqs(4), ierr )
C
      call mpi_waitall(4, reqs, MPI_STATUSES_IGNORE, ierr )
      return
      end
