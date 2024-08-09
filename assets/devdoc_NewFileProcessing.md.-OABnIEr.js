import{_ as e,c as t,o as a,V as i}from"./chunks/framework.gBlNPWt_.js";const u=JSON.parse('{"title":"Storage planes","description":"","frontmatter":{},"headers":[],"relativePath":"devdoc/NewFileProcessing.md","filePath":"devdoc/NewFileProcessing.md","lastUpdated":1723205458000}'),o={name:"devdoc/NewFileProcessing.md"},r=i(`<p>Documentation about the new file work.</p><p>YAML files. The following are the YAML definitions which are used to serialize file information from dali/external store to the engines and if necessary to the worker nodes.</p><h1 id="storage-planes" tabindex="-1">Storage planes <a class="header-anchor" href="#storage-planes" aria-label="Permalink to &quot;Storage planes&quot;">​</a></h1><p>This is already covered in the deployed helm charts. It has been extended and rationalized slightly.</p><p>storage:</p><p>: hostGroups: - name: &lt;required&gt; hosts: [ .... ] - name: &lt;required&gt; hostGroup: &lt;name&gt; count: &lt;unsigned:#hosts&gt; # how many hosts within the host group are used ?(default is number of hosts) offset: &lt;unsigned:0&gt; # index of first host included in the derived group delta: &lt;unsigned:0&gt; # first host within the range[offset..offset+count-1] in the derived group</p><pre><code>planes:

:   name: \\&lt;required\\&gt; prefix: \\&lt;path\\&gt; \\# Root directory for
    accessing the plane (if pvc defined), or url to access plane.
    numDevices: 1 \\# number of devices that are part of the plane
    hostGroup: \\&lt;name\\&gt; \\# Name of the host group for bare metal
    hosts: \\[ host-names \\] \\# A list of host names for bare metal
    secret: \\&lt;secret-id\\&gt; \\# what secret is required to access the
    files. options: \\# not sure if it is needed
</code></pre><p>Changes: * The replication information has been removed from the storage plane. It will now be specified on the thor instance indicating where (if anywhere) files are replicated. * The hash character (#) in a prefix or a secret name will be substituted with the device number. This replaces the old includeDeviceInPath property. This allows more flexible device substition for both local mounts and storage accounts. The number of hashes provides the default padding for the device number. (Existing Helm charts will need to be updated to follow these new rules.) * Neither thor or roxie replication is special cased. They are represented as multiple locations that the file lives (see examples below). Existing baremetal environments would be mapped to this new representation with implicit replication planes. (It is worth checking the mapping to roxie is fine.)</p><h1 id="files" tabindex="-1">Files <a class="header-anchor" href="#files" aria-label="Permalink to &quot;Files&quot;">​</a></h1><p>file: - name: &lt;logical-file-name&gt; format: &lt;type&gt; # e.g. flat, csv, xml, key, parquet meta: &lt;binary&gt; # (opt) format of the file, (serialized hpcc field information). metaCrc: &lt;unsigned&gt; # hash of the meta numParts # How many file parts. singlePartNoSuffix: &lt;boolean&gt; # Does a single part file include .part_1_of_1? numRows: # total number of rows in the file (if known) rawSize: # total uncompressed size diskSize # is this useful? when binary copying? planes: [] # list of storage planes that the file is stored on. tlk: # ???Should the tlk be stored in the meta and returned? splitType: &lt;split-format&gt; # Are there associated split points, and if so what format? (And if so what variant?)</p><p>#options relating to the format of the input file:</p><p>: grouped: &lt;boolean&gt; # is the file grouped? compressed: &lt;boolean&gt; blockCompressed: &lt;boolean&gt; formatOptions: # Any options that relate to the file format e.g. csvTerminator. These are nested because they can be completely free format recordSize: # if a fixed size record. Not really sure it is useful</p><pre><code>part: \\# optional information about each of the file parts (Cannot
implement virtual file position without this) - numRows: \\&lt;count\\&gt;
\\# number of rows in the file part rawSize: \\&lt;size\\&gt; \\# uncompressed
size of the file part diskSize: \\&lt;size\\&gt; \\# size of the part on disk
</code></pre><p># extra fields that are used to return information from the file lookup service</p><blockquote><p>missing: &lt;boolean&gt; # true if the file could not be found external: &lt;boolean&gt; # filename of the form external:: or plane:</p></blockquote><p>If the information needs to be signed to be passed to dafilesrv for example, the entire structure of (storage, files) is serialized, and compressed, and that then signed.</p><h1 id="functions" tabindex="-1">Functions <a class="header-anchor" href="#functions" aria-label="Permalink to &quot;Functions&quot;">​</a></h1><p>Logically executed on the engine, and retrived from dali or in future versions from an esp service (even if for remote reads).</p><p>GetFileInfomation(&lt;logical-filename&gt;, &lt;options&gt;)</p><p>The logical-filename can be any logical name - including a super file, or an implicit superfile.</p><p>options include: * Are compressed sizes needed? * Are signatures required? * Is virtual fileposition (non-local) required? * name of the user</p><p>This returns a structure that provides information about a list of files</p><p>meta:</p><p>: hostGroups: storage: files: secrets: #The secret names are known, how do we know which keys are required for those secrets?</p><p>Some key questions: * Should the TLK be in the dali meta information? [Possibly, but not in first phase. ] * Should the split points be in the dali meta information? [Probably not, but the meta should indicate whether they exist, and if so what format they are. ] * Super files (implicit or explicit) can contain the same file information more than once. Should it be duplicated, or have a flag to indicate a repeat. [I suspect this is fairly uncommon, so duplication would be fine for the first version.] * What storage plane information is serialized back? [ all is simplest. Can optimize later. ]</p><p>NOTE: This doesn&#39;t address the question of writing to a disk file...</p><hr><p>Local class for interpreting the results. Logically executed on the manager, and may gather extra information that will be serialized to all workers. The aim is that the same class implementations are used by all the engines (and fileview in esp).</p><p>MasterFileCollection : RemoteFileCollection : FileCollection(eclReadOptions, eclFormatOptions, wuid, user, expectedMeta, projectedMeta); MasterFileCollection //Master has access to dali RemoteFileCollection : has access to remote esp // think some more</p><p>FileCollection::GatherFileInformation(&lt;logical-filename&gt;, gatherOptions); - potentially called once per query. - class is responsible for optimizing case where it matches the previous call (e.g. in a child query). - possibly responsible for retrieving the split points ()</p><p>Following options are used to control whether split points are retrieved when file information is gathered * number of channels reading the data? * number of strands reading each channel? * preserve order?</p><p>gatherOptions: * is it a temporary file?</p><p>This class serializes all information to every worker, where it is used to recereate a copy of the master filecollection. This will contain information derived from dali, and locally e.g. options specified in the activity helper. Each worker has a complete copy of the file information. (This is similar to dafilesrv with security tokens.)</p><p>The files that are actually required by a worker are calculated by calling the following function. (Note the derived information is not serialized.)</p><p>FilePartition FileCollection::calculatePartition(numChannels, partitionOptions)</p><p>partitionOptions: * number of channels reading the data? * number of strands reading each channel? * which channel? * preserve order? * myIP</p><p>A file partition contains a list of file slices:</p><p>class FileSlice (not serialized) { IMagicRowStream * createRowStream(filter, ...); // MORE! File * logicalFile; offset_t startOffset; offset_t endOffset; };</p><p>Things to bear in mind: - Optimize same file reused in a child query (filter likely to change) - Optimize same format reused in a child query (filename may be dynamic) - Intergrating third party file formats and distributed file systems may require extra information. - optimize reusing the format options. - ideally fail over to a backup copy midstream.. and retry in failed read e.g. if network fault</p><h1 id="examples" tabindex="-1">Examples <a class="header-anchor" href="#examples" aria-label="Permalink to &quot;Examples&quot;">​</a></h1><p>Example definition for a thor400, and two thor200s on the same nodes:</p><p>hostGroup: - name: thor400Group host: [node400_01,node400_02,node400_03,...node400_400]</p><p>storage:</p><p>: planes: #Simple 400 way thor - name: thor400 prefix: /var/lib/HPCCSystems/thor400 hosts: thor400Group #The storage plane used for replicating files on thor. - name: thor400_R1 prefix: /var/lib/HPCCSystems/thor400 hosts: thor400Group offset: 1 # A 200 way thor using the first 200 nodes as the thor 400 - name: thor200A prefix: /var/lib/HPCCSystems/thor400 hosts: thor400Group size: 200 # A 200 way thor using the second 200 nodes as the thor 400 - name: thor200B prefix: /var/lib/HPCCSystems/thor400 hosts: thor400Group size: 200 start: 200 # The replication plane for a 200 way thor using the second 200 nodes as the thor 400 - name: thor200B_R1 prefix: /var/lib/HPCCSystems/thor400 hosts: thor400Group size: 200 start: 200 offset: 1 # A roxie storage where 50way files are stored on a 100 way roxie - name: roxie100 prefix: /var/lib/HPCCSystems/roxie100 hosts: thor400Group size: 50 # The replica of the roxie storage where 50way files are stored on a 100 way roxie - name: roxie100_R1 prefix: /var/lib/HPCCSystems/thor400 hosts: thor400Group start: 50 size: 50</p><p>device = (start + (part + offset) % size;</p><p>size &lt;= numDevices offset &lt; numDevices device &lt;= numDevices;</p><p>There is no special casing of roxie replication, and each file exists on multiple storage planes. All of these should be considered when determining which is the best copy to read from a particular engine node.</p><p>Creating storage planes from an existing systems [implemented]</p><h2 id="milestones" tabindex="-1">Milestones: <a class="header-anchor" href="#milestones" aria-label="Permalink to &quot;Milestones:&quot;">​</a></h2><p>a) Create baremetal storage planes [done]</p><p>b) [a] Start simplifying information in dali meta (e.g. partmask, remove full path name) <em>c) [a] Switch reading code to use storageplane away from using dali path and environment paths - in ALL disk reading and writing code - change numDevices so it matches the container d) [c] Convert dali information from using copies to multiple groups/planes</em>e) [a] Reimplement the current code to create an IPropertyTree from dali file information (in a form that can be reused in dali) *f) [e] Refactor existing PR to use data in an IPropertyTree and cleanly separate the interfaces. g) Switch hthor over to using the new classes by default and work through all issues h) Refactor stream reading code. Look at the spark interfaces for inspiration/compatibility i) Refactor disk writing code into common class? j) [e] create esp service for accessing meta information k) [h] Refactor and review azure blob code l) [k] Re-implement S3 reading and writing code.</p><p>m) Switch fileview over to using the new classes. (Great test they can be used in another context + fixes a longstanding bug.)</p><p>) Implications for index reading? Will they end up being treated as a normal file? Don&#39;t implement for 8.0, although interface may support it.</p><p>*) My primary focus for initial work.</p><h1 id="file-reading-refactoring" tabindex="-1">File reading refactoring <a class="header-anchor" href="#file-reading-refactoring" aria-label="Permalink to &quot;File reading refactoring&quot;">​</a></h1><p>Buffer sizes: - storage plane specifies an optimal reading minimum - compression may have a requirement - the use for the data may impose a requirement e.g. a subset of the data, or only fetching a single record</p><ul><li>parallel disk reading may want to read a big chunk, but then process in sections. groan.</li></ul><p>Look at lambda functions to create split points for a file. Can we use the java classes to implement it on binary files (and csv/xml)?</p><p>*****************<strong>* Reading classes and meta information</strong>****************** meta comes from a combination of the information in dfs and the helper</p><p>The main meta information uses the same structure that is return by the function that returns file infromation from dali. The format specific options are contained in a nested attribute so they can be completely arbitrary</p><p>The helper class also generates a meta structure. Some options fill in root elements - e.g. compressed. Some fill in a new section (hints: @x=y). The format options are generated from the paramaters to the dataset format.</p><p>note normally there is only a single (or very few) files, so merging isn&#39;t too painful. queryMeta() queryOptions() rename meta to format? ???</p><h1 id="dfu-server" tabindex="-1">DFU server <a class="header-anchor" href="#dfu-server" aria-label="Permalink to &quot;DFU server&quot;">​</a></h1><p>Where does DFUserver fit in in a container system?</p><p>DFU has the following main functionality in a bare metal system: a) Spray a file from a 1 way landing zone to an N-way thor b) Convert file format when spraying. I suspect utf-16-&gt;utf8 is the only option actually used. c) Spray multiple files from a landing zone to a single logical file on an N-way thor d) Copy a logical file from a remote environment e) Despray a logical file to an external landing zone. f) Replicate an existing logical file on a given group. g) Copy logical files between groups h) File monitoring i) logical file operations j) superfile operations</p><p>ECL has the ability to read a logical file directly from a landingzone using &#39;FILE::&lt;ip&gt;&#39; file syntax, but I don&#39;t think it is used very frequently.</p><p>How does this map to a containerized system? I think the same basic operations are likely to be useful. a) In most scenarios Landing zones are likely to be replaced with (blob) storage accounts. But for security reasons these are likely to remain distinct from the main location used by HPCC to store datasets. (The customer will have only access keys to copy files to and from those storage accounts.) The containerized system has a way for ECL to directly read from a blob storage account (&#39;PLANE::&lt;plane&#39;), but I imagine users will still want to copy the files in many situations to control the lifetime of the copies etc. b) We still need a way to convert from utf16 to utf8, or extend the platform to allow utf16 to be read directly. c) This is still equally useful, allowing a set of files to be stored as a single file in a form that is easy for ECL to process. d) Important for copying data from an existing bare metal system to the cloud, and from a cloud system back to a bare metal system. e) Useful for exporting results to customers f+g) Essentially the same thing in the cloud world. It might still be useful to have h) I suspect we will need to map this to cloud-specific apis. i+j) Just as applicable in the container world.</p><p>Broadly, landing zones in bare metal map to special storage planes in containerized, and groups also map to more general storage planes.</p><p>There are a couple of complications connected with the implementation:</p><ol><li>Copying is currently done by starting an ftslave process on either the source or the target nodes. In the container world there is no local node, and I think we would prefer not to start a process in order to copy each file. 2) Copying between storage groups should be done using the cloud provider api, rather than transferring data via a k8s job.</li></ol><p>Suggestions:</p><ul><li>Have a load balanced dafilesrv which supports multiple replicas. It would have a secure external service, and an internal service for trusted components.</li><li>Move the ftslave logic into dafilesrv. Move the current code for ftslave actions into dafilesrv with new operations.</li><li>When copying from/to a bare metal system the requests are sent to the dafilesrv for the node that currently runs ftslave. For a container system the requests are sent to the loadbalanced service.</li><li>It might be possible to migrate to lamda style functions for some of the work...</li><li>A later optimization would use a cloud service where it was possible.</li><li>When local split points are supported it may be better to spray a file 1:1 along with partition information. Even without local split points it may still be better to spray a file 1:1 (cheaper).</li><li>What are the spray targets? It may need to be storage plane + number of parts, rather than a target cluster. The default number of parts is the #devices on the storage plane.</li></ul><p>=&gt; Milestones a) Move ftslave code to dafilesrv (partition, pull, push) [Should be included in 7.12.x stream to allow remote read compatibility?] b) Create a dafilesrv component to the helm charts, with internal and external services. c) use storage planes to determine how files are sprayed etc. (bare-metal, #devices) Adapt dfu/fileservices calls to take (storageplane,number) instead of cluster. There should already be a 1:1 mapping from existing cluster to storage planes in a bare-metal system, so this may not involve much work. [May also need a flag to indicate if ._1_of_1 is appended?] d) Select correct dafilesrv for bare-metal storage planes, or load balanced service for other. (May need to think through how remote files are represented.)</p><p>=&gt; Can import from a bare metal system or a containerized system using command line??</p><p>: NOTE: Bare-metal to containerized will likely need push operations on the bare-metal system. (And therefore serialized security information) This may still cause issues since it is unlikely containerized will be able to pull from bare-metal. Pushing, but not creating a logical file entry on the containerized system should be easier since it can use a local storage plane definition.</p><p>e) Switch over to using the esp based meta information, so that it can include details of storage planes and secrets.</p><p>: [Note this would also need to be in 7.12.x to allow remote export to containerized, that may well be a step too far]</p><p>f) Add option to configure the number of file parts for spray/copy/despray g) Ensure that eclwatch picks up the list of storage planes (and the default number of file parts), and has ability to specify #parts.</p><p>Later: h) plan how cloud-services can be used for some of the copies i) investigate using serverless functions to calculate split points. j) Use refactored disk read/write interfaces to clean up read and copy code. k) we may not want to expose access keys to allow remote reads/writes - in which they would need to be pushed from a bare-metal dafilesrv to a containerized dafilesrv.</p><p>Other dependencies: * Refactored file meta information. If this is switching to being plane based, then the meta information should also be plane based. Main difference is not including the path in the meta information (can just be ignored) * esp service for getting file information. When reading remotely it needs to go via this now...</p>`,80),s=[r];function n(l,p,h,c,d,f){return a(),t("div",null,s)}const g=e(o,[["render",n]]);export{u as __pageData,g as default};
