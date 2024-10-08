import { alphanumCompare } from "../../Utility";
import { BaseRow, QueryOptions, QueryRequest, QuerySort } from "../Store";

function createSortFunc<T extends BaseRow>(sortSet: QuerySort<T>, alphanumColumns: { [id: string]: boolean }) {
	return typeof sortSet == "function" ? sortSet : function (a, b) {
		for (let i = 0; sortSet[i]; i++) {
			const sort = sortSet[i];
			if (alphanumColumns[sort.attribute as string]) {
				const cmp = alphanumCompare(a[sort.attribute], b[sort.attribute], true, sort.descending);
				if (cmp !== 0) {
					return cmp;
				}
			} else {
				let aValue = a[sort.attribute];
				let bValue = b[sort.attribute];
				// valueOf enables proper comparison of dates
				aValue = aValue != null ? aValue.valueOf() : aValue;
				bValue = bValue != null ? bValue.valueOf() : bValue;
				if (aValue != bValue) {
					return !!sort.descending == (aValue == null || aValue > bValue) ? -1 : 1;
				}
			}
			return 0;
		}
	};
}
export function SimpleQueryEngine<R extends BaseRow, T extends BaseRow>(_query?: QueryRequest<R>, options?: QueryOptions<T>) {
	// summary:
	//		Simple query engine that matches using filter functions, named filter
	//		functions or objects by name-value on a query object hash
	//
	// description:
	//		The SimpleQueryEngine provides a way of getting a QueryResults through
	//		the use of a simple object hash as a filter.  The hash will be used to
	//		match properties on data objects with the corresponding value given. In
	//		other words, only exact matches will be returned.
	//
	//		This function can be used as a template for more complex query engines;
	//		for example, an engine can be created that accepts an object hash that
	//		contains filtering functions, or a string that gets evaluated, etc.
	//
	//		When creating a new dojo.store, simply set the store's queryEngine
	//		field as a reference to this function.
	//
	// query: Object
	//		An object hash with fields that may match fields of items in the store.
	//		Values in the hash will be compared by normal == operator, but regular expressions
	//		or any object that provides a test() method are also supported and can be
	//		used to match strings by more complex expressions
	//		(and then the regex's or object's test() method will be used to match values).
	//
	// options: dojo/store/api/Store.QueryOptions?
	//		An object that contains optional information such as sort, start, and count.
	//
	// returns: Function
	//		A function that caches the passed query under the field "matches".  See any
	//		of the "query" methods on dojo.stores.
	//
	// example:
	//		Define a store with a reference to this engine, and set up a query method.
	//
	//	|	var myStore = function(options){
	//	|		//	...more properties here
	//	|		this.queryEngine = SimpleQueryEngine;
	//	|		//	define our query method
	//	|		this.query = function(query, options){
	//	|			return QueryResults(this.queryEngine(query, options)(this.data));
	//	|		};
	//	|	};

	// create our matching query function
	let query: any = _query;
	switch (typeof query) {
		default:
			throw new Error("Can not query with a " + typeof query);
		case "object":
		case "undefined":
			const queryObject = query;
			query = function (object) {
				for (const key in queryObject) {
					const required = queryObject[key];
					if (required && required.test) {
						// an object can provide a test method, which makes it work with regex
						if (!required.test(object[key], object)) {
							return false;
						}
					} else if (required != object[key]) {
						return false;
					}
				}
				return true;
			};
			break;
		case "string":
			// named query
			if (!this[query]) {
				throw new Error("No filter function " + query + " was found in store");
			}
			query = this[query];
		// fall through
		case "function":
		// fall through
	}

	function execute(array) {
		// execute the whole query, first we filter
		let results = array.filter(query);
		// next we sort
		const sortSet = options && options.sort;
		if (sortSet) {
			results.sort(createSortFunc(sortSet, options.alphanumColumns));
		}
		// now we paginate
		if (options && (options.start || options.count)) {
			const total = results.length;
			results = results.slice(options.start || 0, (options.start || 0) + (options.count || Infinity));
			results.total = total;
		}
		return results;
	}

	execute.matches = query;
	return execute;
}

