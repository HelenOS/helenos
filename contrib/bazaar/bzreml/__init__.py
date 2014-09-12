#
# Copyright (c) 2009 Martin Decky
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

"""Send emails for commits and repository changes."""

#
# Inspired by bzr-email plugin (copyright (c) 2005 - 2007 Canonical Ltd.,
# distributed under GPL), but no code is shared with the original plugin.
#
# Configuration options:
#  - post_commit_to (destination email address for the commit emails)
#  - post_commit_sender (source email address for the commit emails)
#

import smtplib
import time
import os

from StringIO import StringIO

from email.utils import parseaddr
from email.utils import formatdate
from email.utils import make_msgid
from email.Header import Header
from email.Message import Message
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText

from bzrlib import errors
from bzrlib import revision as _mod_revision
from bzrlib import __version__ as bzrlib_version

from bzrlib.branch import Branch
from bzrlib.diff import DiffTree

def send_smtp(server, sender, to, subject, body):
	"""Send SMTP message"""
	
	connection = smtplib.SMTP()
	
	try:
		connection.connect(server)
	except socket.error, err:
		raise errors.SocketConnectionError(host = server, msg = "Unable to connect to SMTP server", orig_error = err)
	
	sender_user, sender_email = parseaddr(sender)
	payload = MIMEText(body.encode("utf-8"), "plain", "utf-8")
	
	msg = MIMEMultipart()
	msg["From"] = "%s <%s>" % (Header(unicode(sender_user)), sender_email)
	msg["User-Agent"] = "bzr/%s" % bzrlib_version
	msg["Date"] = formatdate(None, True)
	msg["Message-Id"] = make_msgid("bzr")
	msg["To"] = to
	msg["Subject"] = Header(subject)
	msg.attach(payload)
	
	connection.sendmail(sender, [to], msg.as_string())

def config_to(config):
	"""Address the mail should go to"""
	
	return config.get_user_option("post_commit_to")

def config_sender(config):
	"""Address the email should be sent from"""
	
	result = config.get_user_option("post_commit_sender")
	if (result is None):
		result = config.username()
	
	return result

def merge_marker(revision):
	if (len(revision.parent_ids) > 1):
		return " [merge]"
	
	return ""

def iter_reverse_revision_history(repository, revision_id):
	"""Iterate backwards through revision ids in the lefthand history"""
	
	graph = repository.get_graph()
	stop_revisions = (None, _mod_revision.NULL_REVISION)
	return graph.iter_lefthand_ancestry(revision_id, stop_revisions)

def revision_sequence(branch, revision_old_id, revision_new_id):
	"""Calculate a sequence of revisions"""
	
	for revision_ac_id in iter_reverse_revision_history(branch.repository, revision_new_id):
		if (revision_ac_id == revision_old_id):
			break
		
		yield revision_ac_id

def send_email(branch, revision_old_id, revision_new_id, config):
	"""Send the email"""
	
	if (config_to(config) is not None):
		branch.lock_read()
		branch.repository.lock_read()
		try:
			revision_prev_id = revision_old_id
			
			for revision_ac_id in reversed(list(revision_sequence(branch, revision_old_id, revision_new_id))):
				body = StringIO()
				
				revision_ac = branch.repository.get_revision(revision_ac_id)
				revision_ac_no = branch.revision_id_to_revno(revision_ac_id)
				
				repo_name = os.path.basename(branch.base)
				if (repo_name == ''):
					repo_name = os.path.basename(os.path.dirname(branch.base))
				
				committer = revision_ac.committer
				authors = revision_ac.get_apparent_authors()
				date = time.strftime("%Y-%m-%d %H:%M:%S %Z (%a, %d %b %Y)", time.localtime(revision_ac.timestamp))
				
				body.write("Repo: %s\n" % repo_name)
				
				if (authors != [committer]):
					body.write("Author: %s\n" % ", ".join(authors))
				
				body.write("Committer: %s\n" % committer)
				body.write("Date: %s\n" % date)
				body.write("New Revision: %s%s\n" % (revision_ac_no, merge_marker(revision_ac)))
				body.write("New Id: %s\n" % revision_ac_id)
				for parent_id in revision_ac.parent_ids:
					body.write("Parent: %s\n" % parent_id)
				
				body.write("\n")
				
				commit_message = None
				body.write("Log:\n")
				if (not revision_ac.message):
					body.write("(empty)\n")
				else:
					log = revision_ac.message.rstrip("\n\r")
					for line in log.split("\n"):
						body.write("%s\n" % line)
						if (commit_message == None):
							commit_message = line
				
				if (commit_message == None):
					commit_message = "(empty)"
				
				body.write("\n")
				
				tree_prev = branch.repository.revision_tree(revision_prev_id)
				tree_ac = branch.repository.revision_tree(revision_ac_id)
				
				delta = tree_ac.changes_from(tree_prev)
				
				if (len(delta.added) > 0):
					body.write("Added:\n")
					for item in delta.added:
						body.write("    %s\n" % item[0])
				
				if (len(delta.removed) > 0):
					body.write("Removed:\n")
					for item in delta.removed:
						body.write("    %s\n" % item[0])
				
				if (len(delta.renamed) > 0):
					body.write("Renamed:\n")
					for item in delta.renamed:
						body.write("    %s -> %s\n" % (item[0], item[1]))
				
				if (len(delta.kind_changed) > 0):
					body.write("Changed:\n")
					for item in delta.kind_changed:
						body.write("    %s\n" % item[0])
				
				if (len(delta.modified) > 0):
					body.write("Modified:\n")
					for item in delta.modified:
						body.write("    %s\n" % item[0])
				
				body.write("\n")
				
				tree_prev.lock_read()
				try:
					tree_ac.lock_read()
					try:
						diff = DiffTree.from_trees_options(tree_prev, tree_ac, body, "utf8", None, "", "", None)
						diff.show_diff(None, None)
					finally:
						tree_ac.unlock()
				finally:
					tree_prev.unlock()
				
				subject = "[%s] r%d - %s" % (repo_name, revision_ac_no, commit_message)
				send_smtp("localhost", config_sender(config), config_to(config), subject, body.getvalue())
				
				revision_prev_id = revision_ac_id
			
		finally:
			branch.repository.unlock()
			branch.unlock()

def branch_post_change_hook(params):
	"""post_change_branch_tip hook"""
	
	send_email(params.branch, params.old_revid, params.new_revid, params.branch.get_config())

install_named_hook = getattr(Branch.hooks, "install_named_hook", None)
install_named_hook("post_change_branch_tip", branch_post_change_hook, "bzreml")
