# DPDK-MEMZONEMONITOR
DPDK secondary application to monitor the memzone region for updates or changes.

## The application helps to 
- periodically montior memzone zone
- hash is performed for (total length/CACHELINE length).
- HASH can be set to CRC or JHASH. 
- Changes are compared continously.
- Optoin to write to log file is added
