# modules needed for ATPESC 2023 I/O examples on theta.alcf.anl.gov
module add cray-parallel-netcdf
module add cray-hdf5-parallel
module add cray-netcdf-hdf5parallel
module add darshan

# use conda + venv to provide PyDarshan
module use /soft/modulefiles ; module load conda; conda activate base
CONDA_NAME=$(echo ${CONDA_PREFIX} | tr '\/' '\t' | sed -E 's/mconda3|\/base//g' | awk '{print $NF}')
VENV_DIR="/eagle/projects/ATPESC2024/usr/$USER/venvs/${CONDA_NAME}"
mkdir -p "${VENV_DIR}"
python -m venv "${VENV_DIR}" --system-site-packages
source "${VENV_DIR}/bin/activate"
PIP_DISABLE_PIP_VERSION_CHECK=1 pip install -q darshan
