#include <string.h>
#include <assert.h>
#include <hdf5.h>
#include <mpi.h>

#define BUFSIZE 512

int main(int argc, char **argv)
{
    int nprocs, rank;
    size_t len;
    MPI_Offset offset=0, varlen;
    MPI_Info info;
    char buf[BUFSIZE];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Info_create(&info);

    snprintf(buf, BUFSIZE, "Hello from rank %d of %d\n", rank, nprocs);
    len = strlen(buf);

    /* each process figures out where to write its string */
    MPI_Exscan(&len, &offset, 1, MPI_OFFSET, MPI_SUM, MPI_COMM_WORLD);

    /* and then the last process tells everyone else how big it ended up being */
    varlen = offset+len;
    MPI_Bcast(&varlen, 1, MPI_OFFSET, nprocs-1, MPI_COMM_WORLD);

    /* HDF5 stuff starts here:
     * library uses "property lists" to set options, adjust tuning parmeters */
    hid_t file_access_property_list;
    herr_t status;

    /* here we adjust "file access" properties:
     * - collective metadata for both read and write
     * - use MPI-IO for I/O */
    file_access_property_list = H5Pcreate(H5P_FILE_ACCESS);
    assert(file_access_property_list > 0);
    status = H5Pset_all_coll_metadata_ops(file_access_property_list, 1);
    assert(status >= 0);
    status = H5Pset_coll_metadata_write(file_access_property_list, 1);
    assert(status >= 0);
    status = H5Pset_fapl_mpio(file_access_property_list, MPI_COMM_WORLD, info);
    assert(status >= 0);

    hid_t file;
    file = H5Fcreate(argv[1], H5F_ACC_TRUNC, H5P_DEFAULT, file_access_property_list);

    /* finally we can write it all out */
    /* in this simple example everyone writes their string to a 1d dataset
     * HDF5 support variable length arrays ("ragged arrays") but these
     * datatypes have odd interactions with parallel i/o */

    /* like writing to a plain file, we'll create one big variable and everyone
     * can write their string to the right (non-overlapping) place in the file */
    hid_t dataset, datatpye, file_space;
    hsize_t size=varlen;

    file_space = H5Screate_simple(1, &size, NULL);

    /* an HDF5 'dataset' describes the data type of a data space */
    datatpye = H5Tcopy(H5T_NATIVE_CHAR);
    dataset = H5Dcreate(file, "hello", datatpye, file_space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    /* the memory side is easier: it's just a contiguous region */
    hid_t mem_space;
    hsize_t mem_len=len;
    mem_space = H5Screate_simple(1, &mem_len, NULL);

    /* remember we got 'offset' from the MPI_Exscan above */
    hsize_t start=offset, count=len;
    status = H5Sselect_hyperslab(file_space, H5S_SELECT_SET, &start, NULL, &count, NULL);

    /* similar to the file access property list above, we are going to populate
     * a "data transfer" property list to select a different method for
     * writing.  In this case we will request collective I/O */
    hid_t dataset_transfer_property_list;

    dataset_transfer_property_list = H5Pcreate(H5P_DATASET_XFER);
    status = H5Pset_dxpl_mpio(dataset_transfer_property_list, H5FD_MPIO_COLLECTIVE);

    /* finally we can write it all out */
    H5Dwrite(dataset, H5T_NATIVE_CHAR, mem_space, file_space,
	    dataset_transfer_property_list, buf);

    H5Dclose(dataset);
    H5Sclose(mem_space);
    H5Sclose(file_space);
    H5Fclose(file);


    MPI_Finalize();
}
