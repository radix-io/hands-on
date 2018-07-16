C SLIDE: 2D Life Code Walkthrough */
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
       integer tmp_prev, tmp_next, tmp_left, tmp_right, sizeofint
       integer tmp_LCols, tmp_GFirstRow, tmp_GFirstCol, tmp_LRows
       integer nprocs
       integer matrix_win, matrix2_win, above_LRows, left_LRows,            &
     &       right_LRows, left_LCols, right_LCols
       common /mlife2dwin/ matrix_win, matrix2_win, above_LRows,            &
     &       left_LRows, right_LRows, left_LCols, right_LCols
       save /mlife2dwin/
C
       exch_above = above
       exch_below = below
       exch_left  = left
       exch_right = right
C
C create windows 
       call mpi_type_size( MPI_INTEGER, sizeofint, ierr )
       call mpi_win_create(matrix, MaxLRows*(LCols+2)*sizeofint,          &
     &               sizeofint, MPI_INFO_NULL, comm, matrix_win, ierr )

       call mpi_win_create(matrix2, MaxLRows*(LCols+2)*sizeofint,         &
     &               sizeofint, MPI_INFO_NULL, comm, matrix2_win, ierr )

C for one-sided communication, we need to know the number of
C local rows in rank exch_above and the number of local 
C columns in rank exch_left and exch_right in order to do 
C the puts into the right locations in memory.
    
       call mpi_comm_size(comm, nprocs, ierr )

       if (exch_above .eq. MPI_PROC_NULL) then
            above_LRows = 0
       else
            call MLIFE_MeshDecomp(exch_above, nprocs,                      &
     &                rows, cols,                                          &
     &                tmp_left, tmp_right, tmp_prev, tmp_next,             &
     &                above_LRows, tmp_LCols, tmp_GFirstRow,               &
     &                tmp_GFirstCol)
       endif

       if (exch_left .eq. MPI_PROC_NULL) then
           left_LCols = 0
       else 
           call MLIFE_MeshDecomp(exch_left, nprocs,                        &
     &                rows, cols,                                          &
     &                tmp_left, tmp_right, tmp_prev, tmp_next,             &
     &                tmp_LRows, left_LCols, tmp_GFirstRow,                &
     &                tmp_GFirstCol)
       endif

       if (exch_right .eq. MPI_PROC_NULL) then
            right_LCols = 0
       else 
            call MLIFE_MeshDecomp(exch_right, nprocs,                      &
     &               rows, cols,                                          &
     &               tmp_left, tmp_right, tmp_prev, tmp_next,             &
     &               tmp_LRows, right_LCols, tmp_GFirstRow,               &
     &               tmp_GFirstCol)
       endif
       end
C
       subroutine MLIFE_exchange_finalize()
       implicit none
       include 'mpif.h'
       integer ierr
       integer matrix_win, matrix2_win, above_LRows, left_LRows,            &
     &       right_LRows, left_LCols, right_LCols
       common /mlife2dwin/ matrix_win, matrix2_win, above_LRows,            &
     &       left_LRows, right_LRows, left_LCols, right_LCols
       save /mlife2dwin/
C
       call mpi_win_free( matrix_win, ierr )
       call mpi_win_free( matrix2_win, ierr )
       end
C       
       subroutine MLIFE_exchange(matrix, LRows, LCols, parity)
       implicit none
       include 'mpif.h'
       include 'mlife2df.h'
       integer LRows, LCols, matrix(0:MaxLRows-1,0:LCols+1), parity
       integer ierr, win, vectype
C      disp may need to be integer*8 on 64-bit platforms
       integer disp
       integer matrix_win, matrix2_win, above_LRows, left_LRows,            &
     &       right_LRows, left_LCols, right_LCols
       common /mlife2dwin/ matrix_win, matrix2_win, above_LRows,            &
     &       left_LRows, right_LRows, left_LCols, right_LCols
       save /mlife2dwin/
       save vectype
       data vectype/MPI_DATATYPE_NULL/
C
C     Send and receive boundary information

C Find the right window object 
       if (parity .eq. 1) then
           win = matrix_win
       else
           win = matrix2_win
       endif

C create datatype if not already created 
      if (vectype .eq. MPI_DATATYPE_NULL) then
          call mpi_type_vector(LCols, 1, MaxLRows, MPI_INTEGER,            &
     &                         vectype, ierr )
          call mpi_type_commit(vectype, ierr )
      endif
C
C Because we assume MaxLRows is the same on all processes, we
C only need the one vector type (the number of local columns will 
C be the same as the above and below tiles)
C
      call mpi_win_fence( MPI_MODE_NOPRECEDE, win, ierr )

C first put the top, bottom edges
      disp = MaxLRows + (above_LRows + 1)
      call mpi_put(matrix(1,1), 1, vectype, exch_above, disp, 1,           &
     &	    vectype, win, ierr )

      disp = MaxLRows
      call mpi_put(matrix(LRows,1), 1, vectype, exch_below, disp, 1,      &
     &      vectype, win, ierr )

C Complete the top/bottom transfers for the diagonal trick 
      call mpi_win_fence( 0, win, ierr )

C now put the left, right edges (including the diagonal points) 
      disp = MaxLRows * (left_Lcols + 1)
      call mpi_put(matrix(0,1), LRows + 2, MPI_INTEGER, exch_left,         &
     &      disp, LRows + 2, MPI_INTEGER, win, ierr )

      disp = 0
      call mpi_put(matrix(0,LCols), LRows + 2, MPI_INTEGER, exch_right,   &
     &            disp, LRows + 2, MPI_INTEGER, win, ierr )

      call mpi_win_fence(MPI_MODE_NOSTORE + MPI_MODE_NOPUT +                &
     &                   MPI_MODE_NOSUCCEED, win, ierr )
      end
