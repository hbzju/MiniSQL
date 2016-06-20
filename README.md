# minisql
<p>Developer: Zhang Jin / Wang Haobo</p>
<p>SQL Example</p>
<ul>
<li>Create Table:<br>
<pre><code>create table [Table Name] (
  [Attribute1] [type],
  [Attribute2] [type],
  ...
  primary key([Attribute?])
  );</pre></code>
</li>

<li>Create Index:<br>
<pre><code>create Index [Index Name] on [Table Name] ([Attribute])</pre></code>
</li>

<li>Select:<br>
<pre><code>select * from [Table Name] ( where [Attribute] [Condition] [Value] );</pre></code>
</li>

<li>Delete:<br>
<pre><code>delete from [Table Name] ( where [Attribute] [Condition] [Value] );</pre></code>
</li>

</ul>

Condition:{'>' '<' '=' '<>' '>=' '<='}
Type:{char float int}
<strong>Do not miss ' ' and sometimes '\n' will make some mistakes.</strong>
