#include "hdf5.h"

/************/
/* Typedefs */
/************/
typedef struct H5VL_prov_dataset_info_t dataset_prov_info_t;
typedef struct H5VL_prov_group_info_t group_prov_info_t;
typedef struct H5VL_prov_datatype_info_t datatype_prov_info_t;
typedef struct H5VL_prov_attribute_info_t attribute_prov_info_t;
typedef struct H5VL_prov_file_info_t file_prov_info_t;


/* candice added for tracking object access start */
typedef struct FileTracker file_list_t;
typedef struct DsetTracker dset_list_t;

typedef struct FileTracker {
    char * name;
    unsigned long sorder_id;
    unsigned long porder_id;
    unsigned long time; // at self create time
    // size_t size;
    // char op_type[10]; // create/open/close
    // int access_cnt;
    dset_list_t *dset;
    file_list_t *next;
} file_list_t;


typedef struct DsetTracker {
    char * name;
    char * pfile_name;
    unsigned long sorder_id;
    unsigned long porder_id;
    unsigned long pfile_sorder_id;
    unsigned long pfile_porder_id;
    unsigned long time; // at self create time
    // size_t size;
    char obj_type[10]; // read/write/blob_put/blob_get
    // int access_cnt;
    dset_list_t *next;
} dset_list_t;

/* candice added for tracking object access end */

typedef struct ProvenanceHelper {
    /* Provenance properties */
    char* prov_file_path;
    FILE* prov_file_handle;
    Prov_level prov_level;
    char* prov_line_format;
    char user_name[32];
    int pid;
    pthread_t tid;
    char proc_name[64];
    int ptr_cnt;
    int opened_files_cnt;
    file_list_t * fhead;
    file_list_t * fcurr;
    file_prov_info_t* opened_files;//linkedlist,
} prov_helper_t;

typedef struct H5VL_provenance_t {
    hid_t  under_vol_id;        /* ID for underlying VOL connector */
    void  *under_object;        /* Info object for underlying VOL connector */
    H5I_type_t my_type;         /* obj type, dataset, datatype, etc. */
    prov_helper_t *prov_helper; /* pointer shared among all layers, one per process. */
    void *generic_prov_info;    /* Pointer to a class-specific prov info struct. */
                                /* Should be cast to layer-specific type before use, */
                                /* such as file_prov_info, dataset_prov_info. */
} H5VL_provenance_t;

/* The PROVENANCE VOL wrapper context */
typedef struct H5VL_provenance_wrap_ctx_t {
    prov_helper_t *prov_helper; /* shared pointer */
    hid_t under_vol_id;         /* VOL ID for under VOL */
    void *under_wrap_ctx;       /* Object wrapping context for under VOL */
    file_prov_info_t *file_info;
    unsigned long file_no;
    hid_t dtype_id;             /* only used by datatype */
} H5VL_provenance_wrap_ctx_t;

//======================================= statistics =======================================
//typedef struct H5VL_prov_t {
//    void   *under_object;
//    char* func_name;
//    int func_cnt;//stats
//} H5VL_prov_t;

struct H5VL_prov_file_info_t {//assigned when a file is closed, serves to store stats (copied from shared_file_info)
    prov_helper_t* prov_helper;  //pointer shared among all layers, one per process.
    char* file_name;
    char* intent;
    unsigned long file_no;
    unsigned long sorder_id;
    unsigned long porder_id;
    hsize_t file_size;
#ifdef H5_HAVE_PARALLEL
    // Only present for parallel HDF5 builds
    MPI_Comm mpi_comm;           // Copy of MPI communicator for file
    MPI_Info mpi_info;           // Copy of MPI info for file
    hbool_t mpi_comm_info_valid; // Indicate that MPI Comm & Info are valid
#endif /* H5_HAVE_PARALLEL */
    int ref_cnt;

    /* Currently open objects */
    int opened_datasets_cnt;
    dataset_prov_info_t *opened_datasets;
    int opened_grps_cnt;
    group_prov_info_t *opened_grps;
    int opened_dtypes_cnt;
    datatype_prov_info_t *opened_dtypes;
    int opened_attrs_cnt;
    attribute_prov_info_t *opened_attrs;

    /* Statistics */
    int ds_created;
    int ds_accessed;
    int grp_created;
    int grp_accessed;
    int dtypes_created;
    int dtypes_accessed;

    file_prov_info_t *next;
};

// Common provenance information, for all objects
typedef struct H5VL_prov_object_info_t {
    prov_helper_t *prov_helper;         //pointer shared among all layers, one per process.
    file_prov_info_t *file_info;        // Pointer to file info for object's file
    H5O_token_t token;                  // Unique ID within file for object
    char *name;                         // Name of object within file
                                        // (possibly NULL and / or non-unique)
    int ref_cnt;                        // # of references to this prov info
} object_prov_info_t;

struct H5VL_prov_dataset_info_t {
    object_prov_info_t obj_info;        // Generic prov. info
                                        // Must be first field in struct, for
                                        // generic upcasts to work
    
    char * pfile_name; // parent file name
    H5T_class_t dt_class;               //data type class
    H5S_class_t ds_class;               //data space class
    H5D_layout_t layout;
    unsigned int dimension_cnt;
    hsize_t dimensions[H5S_MAX_RANK];
    size_t dset_type_size;
    hsize_t dset_space_size;            //unsigned long long
    haddr_t dset_offset;
    hsize_t storage_size;
    size_t dset_n_elements;
    ssize_t hyper_nblocks;
    hid_t dset_id;
    hid_t dspace_id;
    unsigned long sorder_id;
    unsigned long porder_id;
    unsigned long pfile_sorder_id;
    unsigned long pfile_porder_id;

    hsize_t total_bytes_read;
    hsize_t total_bytes_written;
    hsize_t total_read_time;
    hsize_t total_write_time;
    int dataset_read_cnt;
    int dataset_write_cnt;
    /* candice added for recording blob start */
    int blob_put_cnt;
    size_t total_bytes_blob_put;
    hsize_t total_blob_put_time;
    int blob_get_cnt;
    size_t total_bytes_blob_get;
    hsize_t total_blob_get_time;
    /* candice added for recording blob end */
#ifdef H5_HAVE_PARALLEL
    int ind_dataset_read_cnt;
    int ind_dataset_write_cnt;
    int coll_dataset_read_cnt;
    int coll_dataset_write_cnt;
    int broken_coll_dataset_read_cnt;
    int broken_coll_dataset_write_cnt;
#endif /* H5_HAVE_PARALLEL */
    int access_cnt;
    
    dataset_prov_info_t *next;
};

struct H5VL_prov_group_info_t {
    object_prov_info_t obj_info;        // Generic prov. info
                                        // Must be first field in struct, for
                                        // generic upcasts to work

    int func_cnt;//stats
//    int group_get_cnt;
//    int group_specific_cnt;

    group_prov_info_t *next;
};

typedef struct H5VL_prov_link_info_t {
    int link_get_cnt;
    int link_specific_cnt;
} link_prov_info_t;

struct H5VL_prov_datatype_info_t {
    object_prov_info_t obj_info;        // Generic prov. info
                                        // Must be first field in struct, for
                                        // generic upcasts to work

    hid_t dtype_id;
    int datatype_commit_cnt;
    int datatype_get_cnt;

    datatype_prov_info_t *next;
};

struct H5VL_prov_attribute_info_t {
    object_prov_info_t obj_info;        // Generic prov. info
                                        // Must be first field in struct, for
                                        // generic upcasts to work

    int func_cnt;//stats

    attribute_prov_info_t *next;
};



