{* $Id: 13_if.tpl,v 1.2 2003/12/20 22:45:07 mclap Exp $ *}
{$a=2;$b=1}

{if:$a==1}
Hello,if_a_1
{elseif:$a==2}
Hello,elseif_a_2

	{if:$b==1}
		$b=1
	{elseif:$b==2}
		$b=2
	{else}
		$b other
	{/if}

{else}
Hello,else_a
{/if}
