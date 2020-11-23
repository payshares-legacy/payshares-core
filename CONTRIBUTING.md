#How to contribute

We're striving to keep master's history with minimal merge bubbles. To achieve this, we're asking PRs to be submitted rebased on top of master.

To keep your local repository in a "rebased" state, simply run:

`git config --global branch.autosetuprebase always` _changes the default for all future branches_

`git config --global branch.master.rebase true` _changes the setting for branch master_

Note: you may still have to run manual "rebase" commands on your branches, to rebase on top of master as you pull changes from upstream.

#Submitting Changes

Please [sign the Contributor License Agreement](https://docs.google.com/forms/d/1g7EF6PERciwn7zfmfke5Sir2n10yddGGSXyZsq98tVY/viewform).

All content, comments, and pull requests must follow the [Payshares Community Guidelines](https://www.payshares.org/community-guidelines/). 

Submit a pull request rebased on top of master

 * Include a descriptive commit message.
 * Changes contributed via pull request should focus on a single issue at a time.
 
At this point you're waiting on us. We like to at least comment on pull requests within one week (and, typically, three business days). We may suggest some changes or improvements or alternatives.
