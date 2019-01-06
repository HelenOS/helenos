/** @addtogroup hbench hbench
 * @brief HelenOS user space benchmarks
 * @ingroup apps
 *
 * @details
 *
 * To add a new benchmark, you need to implement the actual benchmarking
 * code and register it.
 *
 * Registration is done by adding
 * <code>extern benchmark_t bench_YOUR_NAME</code> reference to benchlist.h
 * and by adding it to the array in benchlist.c.
 *
 * The actual benchmark should reside in a separate file (see malloc/malloc1.c
 * for example) and has to (at least) declare one function (the actual
 * benchmark) and fill-in the benchmark_t structure.
 *
 * Fill-in the name of the benchmark, its description and a reference to the
 * benchmark function to the benchmark_t.
 *
 * The benchmarking function has to accept for arguments:
 *  @li stopwatch_t: store the measured data there
 *  @li uint64_t: size of the workload - typically number of inner loops in
 *      your benchmark (used to self-calibrate benchmark size)
 *  @li char * and size_t giving you access to buffer for storing error message
 *  if the benchmark fails (return false from the function itself then)
 *
 * Typically, the structure of the function is following:
 * @code{c}
 * static bool runner(stopwatch_t *stopwatch, uint64_t size,
 *     char *error, size_t error_size)
 * {
 * 	stopwatch_start(stopwatch);
 * 	for (uint64_t i = 0; i < size; i++) {
 * 		// measured action
 * 	}
 * 	stopwatch_stop(stopwatch);
 *
 * 	return true;
 * }
 * @endcode
 */
