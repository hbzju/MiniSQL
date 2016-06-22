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

<li>Drop Table:<br>
<pre><code>Drop [Table Name];</pre></code>
</li>

<li>Drop Index:<br>
<pre><code>Drop [Index Name];</pre></code>
</li>

<li>Execfile:<br>
<pre><code>execfile [File Name];</pre></code>
</li>

</ul>

<p>Condition:{ '>' '<' '=' '<>' '>=' '<=' }</p>
<p>Type:{ char float int }</p>
<p>Other Integrity:{ unique 'primary key' }</p>
<p><strong>Do not miss ' ' and sometimes '\n' will make some mistakes.</strong></p>
