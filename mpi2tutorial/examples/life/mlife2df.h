C -*-; Mode:Fortran; -*-
      common /mlife2d/ opt_rows, opt_cols, opt_iter, opt_prows, 
     $                 opt_pcols, opt_restart_iter
      common /mlife2dc/ opt_prefix
      common /mlife2de/ exch_above, exch_below, exch_left, exch_right,     &
     &                   exch_comm
      save /mlife2d/, /mlife2dc/, /mlife2de/
      integer exch_above, exch_below, exch_left, exch_right, exch_comm
      integer opt_rows, opt_cols, opt_iter, opt_prows, opt_pcols
      integer opt_restart_iter
      character*80 opt_prefix
      integer BORN, DIES, MaxLRows, MaxLCols
      parameter (BORN=1)
      parameter (DIES=0)
      parameter (MaxLRows=100)
      parameter (MaxLCols=100)
