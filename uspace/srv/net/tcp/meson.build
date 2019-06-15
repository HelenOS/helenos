deps = [ 'nettl' ]

_common_src = files(
	'conn.c',
	'inet.c',
	'iqueue.c',
	'ncsim.c',
	'pdu.c',
	'rqueue.c',
	'segment.c',
	'seq_no.c',
	'test.c',
	'tqueue.c',
	'ucall.c',
)

src = files(
	'service.c',
	'tcp.c',
)

test_src = files(
	'test/conn.c',
	'test/iqueue.c',
	'test/main.c',
	'test/pdu.c',
	'test/rqueue.c',
	'test/segment.c',
	'test/seq_no.c',
	'test/tqueue.c',
	'test/ucall.c',
)

src = [ _common_src, src ]
test_src = [ _common_src, test_src ]
