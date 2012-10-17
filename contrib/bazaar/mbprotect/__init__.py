#
# Copyright (c) 2009 Jiri Svoboda
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

"""Main Branch Protection plugin for Bazaar."""

#
# This plugin employs the pre_change_branch_tip hook to protect the main
# branch (a.k.a. linear history) of bzr repositories from unwanted change,
# effectively making the main branch append-only.
#
# Specifically we verify that the new main branch contains the old tip.
# This prevents a branch from inadvertently aquiring the tip of another branch.
#
# Install this plugin in ~/.bazaar/plugins/mbprotect
#
# See also http://trac.helenos.org/trac.fcgi/wiki/BazaarWorkflow
#

import bzrlib.branch
from bzrlib.errors import TipChangeRejected
from bzrlib import revision as _mod_revision

def iter_reverse_revision_history(repository, revision_id):
	"""Iterate backwards through revision ids in the lefthand history"""
	
	graph = repository.get_graph()
	stop_revisions = (None, _mod_revision.NULL_REVISION)
	return graph.iter_lefthand_ancestry(revision_id, stop_revisions)

def pre_change_branch_tip(params):
	repo = params.branch.repository
	
	# Check if the old repository was empty.
	if params.old_revid == 'null:':
		return
	
	# First permitted case is appending changesets to main branch.Look for
	# old tip in new main branch.
	for revision_id in iter_reverse_revision_history(repo, params.new_revid):
		if revision_id == params.old_revid:
			return	# Found old tip
	
	# Another permitted case is backing out changesets. Look for new tip
	# in old branch.
	for revision_id in iter_reverse_revision_history(repo, params.old_revid):
		if revision_id == params.new_revid:
			return	# Found new tip
	
	# Trying to do something else. Reject the change.
	raise TipChangeRejected('Bad tip. Read http://trac.helenos.org/wiki/BazaarWorkflow')

# Install hook.
bzrlib.branch.Branch.hooks.install_named_hook('pre_change_branch_tip',
	pre_change_branch_tip, 'MB-Protect tip check')
