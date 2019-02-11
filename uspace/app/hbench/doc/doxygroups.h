/** @addtogroup hbench hbench
 * @brief User space benchmarks
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
 * The benchmarking function has to accept trhee arguments:
 *  @li bench_env_t: benchmark environment configuration
 *  @li bench_run_t: call bench_run_start and bench_run_stop around the
 *      actual benchmarking code
 *  @li uint64_t: size of the workload - typically number of inner loops in
 *      your benchmark (used to self-calibrate benchmark size)
 *
 * Typically, the structure of the function is following:
 * @code{c}
 * static bool runnerconst bench_env_t const *envbench_run_t *run, uint64_t size)
 * {
 * 	bench_run_start(run);
 * 	for (uint64_t i = 0; i < size; i++) {
 * 		// measured action
 * 		if (something_fails) {
 * 		    return bench_run_fail(run, "oops: %s (%d)", str_error(rc), rc);
 * 		}
 * 	}
 * 	bench_run_stop(run);
 *
 * 	return true;
 * }
 * @endcode
 */
