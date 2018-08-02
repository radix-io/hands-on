#ifndef PNETCDF_UTIL_H
#define PNETCDF_UTIL_H


#define NC_CHECK(status) { \
    int nc_status = status; \
    if (nc_status != NC_NOERR) {\
	perror(ncmpi_strerror(nc_status)); \
	MPI_Abort(MPI_COMM_WORLD, -1); } \
}


#endif
