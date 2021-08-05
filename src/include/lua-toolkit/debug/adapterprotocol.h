//◦ Playrix ◦
// Debug Adapter Protocol spec: https://microsoft.github.io/debug-adapter-protocol/specification
// https://microsoft.github.io/debug-adapter-protocol/
#pragma once
#include <runtime/serialization/runtimevalue.h>
#include <runtime/serialization/serialization.h>
#include <runtime/meta/classinfo.h>
//#include <boost/optional.hpp>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>


namespace Runtime::Dap {

/**
	Base class of requests, responses, and events.
*/
struct ProtocolMessage
{
	static constexpr unsigned InvalidSeq = std::numeric_limits<unsigned>::max();

	static constexpr std::string_view MessageRequest {"request"};
	static constexpr std::string_view MessageEvent {"event"};
	static constexpr std::string_view MessageResponse {"response"};


#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(seq, Serialization::RequiredFieldAttribute{}),
			CLASS_FIELD(type, Serialization::RequiredFieldAttribute{})
		)
	)
#pragma endregion

	/**
		Sequence number (also known as message ID). For protocol messages of type 'request' this ID can be used to cancel the request.
	*/
	unsigned seq = InvalidSeq;

	/**
		Message type.
		Values: 'request', 'response', 'event', etc.
	*/
	std::string type;

	ProtocolMessage() = default;

	ProtocolMessage(unsigned messageSeq, std::string messageType);
};

/**
	A client or debug adapter initiated request.
*/
struct RequestMessage : ProtocolMessage
{
#pragma region Class info
	CLASS_INFO(
		CLASS_BASE(ProtocolMessage),

		CLASS_FIELDS(
			CLASS_FIELD(command, Serialization::RequiredFieldAttribute{}),
			CLASS_FIELD(arguments)
		)
	)
#pragma endregion

	/**
		The command to execute.
	*/
	std::string command;

	/**
		Object containing arguments for the command.
	*/
	RuntimeValue::Ptr arguments;
};

/**
	A debug adapter initiated event.
*/
struct EventMessage : ProtocolMessage
{
#pragma region Class info
	CLASS_INFO(
		CLASS_BASE(ProtocolMessage),

		CLASS_FIELDS(
			CLASS_NAMED_FIELD(eventType, "event", Serialization::RequiredFieldAttribute{})
			// CLASS_FIELD(body)
		)
	)
#pragma endregion

	/**
		Type of event.
	*/
	std::string eventType;

	///**
	//	Event-specific information.
	//*/
	//RuntimeValue::Ptr body;

	EventMessage () = default;

	EventMessage (unsigned messageSeq, std::string_view);
};



template<typename Body = RuntimeValue::Ptr>
struct GenericEventMessage : EventMessage
{
#pragma region Class info
	CLASS_INFO(
		CLASS_BASE(EventMessage),

		CLASS_FIELDS(
			CLASS_FIELD(body)
		)
	)
#pragma endregion

	using EventMessage::EventMessage;

	Body body;
};


/**
	Response for a request.
*/
struct ResponseMessage : ProtocolMessage
{
#pragma region Class info
	CLASS_INFO(
		CLASS_BASE(ProtocolMessage),

		CLASS_FIELDS(
			CLASS_FIELD(request_seq, Serialization::RequiredFieldAttribute{}),
			CLASS_FIELD(success, Serialization::RequiredFieldAttribute{}),
			CLASS_FIELD(command),
			CLASS_FIELD(message)
		)
	)
#pragma endregion

	/**
		Sequence number of the corresponding request.
	*/
	unsigned request_seq = ProtocolMessage::InvalidSeq;

	/**
		Outcome of the request.
			If true, the request was successful and the 'body' attribute may contain the result of the request.
			If the value is false, the attribute 'message' contains the error in short form and the 'body' may contain additional information (see 'ErrorResponse.body.error').
	*/
	bool success = true;

	/**
		The command requested.
	*/
	std::string command;

	/**
		Contains the raw error in short form if 'success' is false.
		This raw error might be interpreted by the frontend and is not shown in the UI.
		Some predefined values exist.
		Values:
			'cancelled': request was cancelled.
			etc.
	*/
	std::optional<std::string> message;


	ResponseMessage () = default;

	ResponseMessage(unsigned messageSeq, const RequestMessage& request);

	ResponseMessage& SetError(std::string_view errorMessage);
};


template<typename Body = RuntimeValue::Ptr>
struct GenericResponseMessage : ResponseMessage
{
#pragma region Class info
	CLASS_INFO(
		CLASS_BASE(ResponseMessage),

		CLASS_FIELDS(
			CLASS_FIELD(body)
		)
	)
#pragma endregion

	using ResponseMessage::ResponseMessage;

	/* Contains request result if success is true and optional error details if success is false. 	*/
	Body body;
};



/**
	The checksum of an item calculated by the specified algorithm.
*/
struct Checksum
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(algorithm, Serialization::RequiredFieldAttribute{}),
			CLASS_FIELD(checksum, Serialization::RequiredFieldAttribute{})
		)
	)
#pragma endregion

	/**
		The algorithm used to calculate this checksum.
		'MD5' | 'SHA1' | 'SHA256' | 'timestamp';
		*/
	std::string algorithm;

	/**
		Value of the checksum.
	*/
	std::string checksum;
};


/* Provides formatting information for a value. */
struct ValueFormat
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(hex)
		)
	)
#pragma endregion

	/* Display the value in hex. */
	std::optional<bool> hex;
};


/**
	Provides formatting information for a stack frame.
*/
struct StackFrameFormat : ValueFormat
{
#pragma region Class info
	CLASS_INFO(
		CLASS_BASE(ValueFormat),

		CLASS_FIELDS(
			CLASS_FIELD(parameters),
			CLASS_FIELD(parameterTypes),
			CLASS_FIELD(parameterNames),
			CLASS_FIELD(parameterValues),
			CLASS_FIELD(line),
			CLASS_FIELD(module),
			CLASS_FIELD(includeAll)
		)
	)
#pragma endregion

	/* Displays parameters for the stack frame. */
	std::optional<bool> parameters;

	/* Displays the types of parameters for the stack frame. */
	std::optional<bool> parameterTypes;

	/* Displays the names of parameters for the stack frame. */
	std::optional<bool> parameterNames;

	/* Displays the values of parameters for the stack frame. */
	std::optional<bool> parameterValues;

	/* Displays the line number of the stack frame. */
	std::optional<bool> line;

	/* Displays the module of the stack frame. */
	std::optional<bool> module;

	/* Includes all stack frames, including those the debug adapter might otherwise hide. */
	std::optional<bool> includeAll;
};

/**
	A Source is a descriptor for source code.
	It is returned from the debug adapter as part of a StackFrame and it is used by clients when specifying breakpoints.
*/

struct Source {

#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(name),
			CLASS_FIELD(path),
			CLASS_FIELD(sourceReference),
			CLASS_FIELD(presentationHint),
			CLASS_FIELD(origin),
			CLASS_FIELD(sources),
			CLASS_FIELD(adapterData),
			CLASS_FIELD(checksums)
		)
	)
#pragma endregion

	/**
		The short name of the source. Every source returned from the debug adapter has a name.
		When sending a source to the debug adapter this name is optional.
	*/
	std::string name;

	/**
		The path of the source to be shown in the UI.
		It is only used to locate and load the content of the source if no sourceReference is specified (or its value is 0).
	*/
	std::string path;

	/**
		If sourceReference > 0 the contents of the source must be retrieved through the SourceRequest (even if a path is specified).
		A sourceReference is only valid for a session, so it must not be used to persist a source.
		The value should be less than or equal to 2147483647 (2^31-1).
	*/
	std::optional<int> sourceReference;

	/**
		An optional hint for how to present the source in the UI.
		A value of 'deemphasize' can be used to indicate that the source is not available or that it is skipped on stepping.
	*/
	std::string presentationHint; // 'normal' | 'emphasize' | 'deemphasize'

	/**
		The (optional) origin of this source: possible values 'internal module', 'inlined content from source map', etc.
	*/
	std::string origin;

	/**
		An optional list of sources that are related to this source. These may be the source that generated this source.
	*/
	std::vector<Source> sources;

	/**
		Optional data that a debug adapter might want to loop through the client.
		The client should leave the data intact and persist it across sessions. The client should not interpret the data.
	*/
	RuntimeValue::Ptr adapterData;

	/* The checksums associated with this file. */
	std::vector<Checksum>checksums;


	Source() = default;

	Source(std::string_view sourcePath);

private:
	friend bool operator == (const Source& left, const Source& right);

	friend bool operator == (const Source& left, std::string_view);
};

/**
	Information about a Breakpoint created in setBreakpoints, setFunctionBreakpoints, setInstructionBreakpoints, or setDataBreakpoints.
*/
struct Breakpoint
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(id),
			CLASS_FIELD(verified),
			CLASS_FIELD(message),
			CLASS_FIELD(source),
			CLASS_FIELD(line),
			CLASS_FIELD(column),
			CLASS_FIELD(endLine),
			CLASS_FIELD(endColumn),
			CLASS_FIELD(instructionReference),
			CLASS_FIELD(offset)
		)
	)
#pragma endregion

	/* An optional identifier for the breakpoint. It is needed if breakpoint events are used to update or remove breakpoints. */
	std::optional<unsigned> id;

	/* If true breakpoint could be set (but not necessarily at the desired location). */
	bool verified;

	/**
		An optional message about the state of the breakpoint.
		This is shown to the user and can be used to explain why a breakpoint could not be verified.
	*/
	std::string message;
	
	/* The source where the breakpoint is located. */
	std::optional<Source> source;

	/* The start line of the actual range covered by the breakpoint. */
	std::optional<unsigned> line;

	/* An optional start column of the actual range covered by the breakpoint. */
	std::optional<unsigned> column;

	/* An optional end line of the actual range covered by the breakpoint. */
	std::optional<unsigned>  endLine;
	
	/** An optional end column of the actual range covered by the breakpoint.
			If no end line is given, then the end column is assumed to be in the start line.
	*/
	std::optional<unsigned> endColumn;

	/* An optional memory reference to where the breakpoint is set. */
	std::string instructionReference;

	/**
		An optional offset from the instruction reference.
		This can be negative.
	*/
	std::optional<unsigned> offset;


	Breakpoint() = default;
};


/**
	Properties of a breakpoint or logpoint passed to the setBreakpoints request.
*/
struct SourceBreakpoint
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(line),
			CLASS_FIELD(column),
			CLASS_FIELD(condition),
			CLASS_FIELD(hitCondition),
			CLASS_FIELD(logMessage)
		)
	)
#pragma endregion

	/* The source line of the breakpoint or logpoint. */
	unsigned line;

	/* An optional source column of the breakpoint. */
	std::optional<unsigned> column;

	/**
		An optional expression for conditional breakpoints.
		It is only honored by a debug adapter if the capability 'supportsConditionalBreakpoints' is true.
	*/
	std::string condition;

	/**
		An optional expression that controls how many hits of the breakpoint are ignored.
		The backend is expected to interpret the expression as needed.
		The attribute is only honored by a debug adapter if the capability 'supportsHitConditionalBreakpoints' is true.
	*/
	std::string hitCondition;

	/**
		If this attribute exists and is non-empty, the backend must not 'break' (stop)
		but log the message instead. Expressions within {} are interpolated.
		The attribute is only honored by a debug adapter if the capability 'supportsLogPoints' is true.
	*/
	std::string logMessage;
};


/* Properties of a breakpoint passed to the setFunctionBreakpoints request. */
struct FunctionBreakpoint
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(name),
			CLASS_FIELD(condition),
			CLASS_FIELD(hitCondition)
		)
	)
#pragma endregion

	/* The name of the function. */
	std::string name;
	/**
		An optional expression for conditional breakpoints.
		It is only honored by a debug adapter if the capability 'supportsConditionalBreakpoints' is true.
*/
	std::string condition;
	/** An optional expression that controls how many hits of the breakpoint are ignored.
			The backend is expected to interpret the expression as needed.
			The attribute is only honored by a debug adapter if the capability 'supportsHitConditionalBreakpoints' is true.
	*/
	std::string hitCondition;
};


/**
*
*/
struct Thread
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(id),
			CLASS_FIELD(name)
		)
	)
#pragma endregion

	/* Unique identifier for the thread. */
	unsigned id = 0;

 /* A name of the thread. */
	std::string name;

	Thread() = default;

	Thread(unsigned threadId, std::string_view threadName);
};



/**
	Arguments for 'setBreakpoints' request.
*/
struct SetBreakpointsArguments
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(source),
			CLASS_FIELD(breakpoints),
			CLASS_FIELD(sourceModified)
		)
	)
#pragma endregion

	/**
		The source location of the breakpoints; either 'source.path' or 'source.reference' must be specified.
	*/
	Source source;

	/**
		The code locations of the breakpoints.
	*/
	std::vector<SourceBreakpoint> breakpoints;
	
	/**
		A value of true indicates that the underlying source has been modified which results in new breakpoint locations.
	*/
	std::optional<bool> sourceModified;
};

/**
	Response to 'setBreakpoints' request.
	Returned is information about each breakpoint created by this request.
	This includes the actual code location and whether the breakpoint could be verified.
	The breakpoints returned are in the same order as the elements of the 'breakpoints'
	(or the deprecated 'lines') array in the arguments.
*/
struct SetBreakpointsResponseBody
{
#pragma region Class info
CLASS_INFO(
	CLASS_FIELDS(
		CLASS_FIELD(breakpoints)
	)
)
#pragma endregion
	/**
		Information about the breakpoints.
		The array elements are in the same order as the elements of the 'breakpoints' (or the deprecated 'lines') array in the arguments.
	*/
	std::vector<Breakpoint> breakpoints;
};


/**
	SetFunctionBreakpoints request; value of command field is 'setFunctionBreakpoints'.
	Replaces all existing function breakpoints with new function breakpoints.
	To clear all function breakpoints, specify an empty array.
	When a function breakpoint is hit, a 'stopped' event (with reason 'function breakpoint') is generated.
	Clients should only call this request if the capability 'supportsFunctionBreakpoints' is true.
*/
struct SetFunctionBreakpointsArguments
{
#pragma region Class info
CLASS_INFO(
	CLASS_FIELDS(
		CLASS_FIELD(breakpoints)
	)
)
#pragma endregion

	std::vector<FunctionBreakpoint> breakpoints;
};


/**
	Response to 'setFunctionBreakpoints' request.
	Returned is information about each breakpoint created by this request.
*/
struct SetFunctionBreakpointsResponseResponseBody
{
#pragma region Class info
CLASS_INFO(
	CLASS_FIELDS(
		CLASS_FIELD(breakpoints)
	)
)
#pragma endregion
	/* Information about the breakpoints. The array elements correspond to the elements of the 'breakpoints' array. */
	std::vector<Breakpoint> breakpoints;
};


struct ThreadsResponseBody
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(threads)
		)
	)
#pragma endregion

	std::vector<Thread> threads;
};



 /**
	Event message for 'stopped' event type.
	The event indicates that the execution of the debuggee has stopped due to some condition.
	This can be caused by a break point previously set, a stepping request has completed, by executing a debugger statement etc.
*/
struct StoppedEventBody
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(reason),
			CLASS_FIELD(description),
			CLASS_FIELD(threadId),
			CLASS_FIELD(preserveFocusHint),
			CLASS_FIELD(text),
			CLASS_FIELD(allThreadsStopped),
			CLASS_FIELD(hitBreakpointIds)
		)
	)
#pragma endregion

	/**
		The reason for the event.
		For backward compatibility this string is shown in the UI if the 'description' attribute is missing (but it must not be translated).
		Values: 'step', 'breakpoint', 'exception', 'pause', 'entry', 'goto', 'function breakpoint', 'data breakpoint', 'instruction breakpoint', etc.
	*/
	std::string reason;

	/* The full reason for the event, e.g. 'Paused on exception'. This string is shown in the UI as is and must be translated. */
	std::optional<std::string> description;

	/* The thread which was stopped. */
	std::optional<unsigned> threadId;

	/* A value of true hints to the frontend that this event should not change the focus. */
	std::optional<bool> preserveFocusHint;

	/* Additional information. E.g. if reason is 'exception', text contains the exception name. This string is shown in the UI. */
	std::optional<std::string> text;

	/**
		If 'allThreadsStopped' is true, a debug adapter can announce that all threads have stopped.
			- The client should use this information to enable that all threads can be expanded to access their stacktraces.
			- If the attribute is missing or false, only the thread with the given threadId can be expanded.
	*/
	std::optional<bool> allThreadsStopped;

	/**
		Ids of the breakpoints that triggered the event. In most cases there will be only a single breakpoint but here are some examples for multiple breakpoints:
			- Different types of breakpoints map to the same location.
			- Multiple source breakpoints get collapsed to the same instruction by the compiler/runtime.
			- Multiple function breakpoints with different function names map to the same location.
	*/
	std::optional<std::vector<unsigned>> hitBreakpointIds;

	
	StoppedEventBody() = default;

	StoppedEventBody(std::string_view eventReason, std::optional<unsigned> eventThread = std::nullopt);

	StoppedEventBody(std::string_view eventReason, std::string_view eventDescription, std::optional<unsigned> eventThread = std::nullopt);
};


/**
	Arguments for 'stackTrace' request.
*/
struct StackTraceArguments
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(threadId),
			CLASS_FIELD(startFrame),
			CLASS_FIELD(levels),
			CLASS_FIELD(format)
		)
	)
#pragma endregion

	/* Retrieve the stacktrace for this thread. */
	unsigned threadId;

	/* The index of the first frame to return; if omitted frames start at 0. */
	std::optional<unsigned> startFrame;

	/* The maximum number of frames to return. If levels is not specified or 0, all frames are returned. */
	std::optional<unsigned> levels;

	/*
		Specifies details on how to format the stack frames.
		The attribute is only honored by a debug adapter if the capability 'supportsValueFormattingOptions' is true.
	*/
	std::optional<StackFrameFormat> format;
};


/**
	A Stackframe contains the source location.
*/
struct StackFrame
{

#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(id),
			CLASS_FIELD(name),
			CLASS_FIELD(source),
			CLASS_FIELD(line),
			CLASS_FIELD(column),
			CLASS_FIELD(endLine),
		)
	)
#pragma endregion

		/**
			An identifier for the stack frame. It must be unique across all threads.
			This id can be used to retrieve the scopes of the frame with the 'scopesRequest' or to restart the execution of a stackframe.
		*/
		unsigned id;

		/* The name of the stack frame, typically a method name. */
		std::string name;

		/* The optional source of the frame. */
		std::optional<Source> source;

		/* The line within the file of the frame. If source is null or doesn't exist, line is 0 and must be ignored. */
		unsigned line = 0;

		/* The column within the line. If source is null or doesn't exist, column is 0 and must be ignored. */
		unsigned column = 0;

		/* An optional end line of the range covered by the stack frame. */
		std::optional<unsigned> endLine;

		/* An optional end column of the range covered by the stack frame. */
		std::optional<unsigned> endColumn;

		/* Indicates whether this frame can be restarted with the 'restart' request. Clients should only use this if the debug adapter supports the 'restart' request (capability 'supportsRestartRequest' is true). */
		std::optional<bool> canRestart;

		/* Optional memory reference for the current instruction pointer in this frame. */
		std::optional<std::string> instructionPointerReference;

		/* The module associated with this frame, if any. */
		std::optional<std::string> moduleId;

		/**
			An optional hint for how to present this frame in the UI.
			A value of 'label' can be used to indicate that the frame is an artificial frame that is used as a visual label or separator. A value of 'subtle' can be used to change the appearance of a frame in a 'subtle' way.
			'normal' | 'label' | 'subtle'
		*/
		std::optional<std::string> presentationHint;


		StackFrame() = default;

		StackFrame(unsigned frameId, std::string_view frameName);
};


/**
	Response to 'stackTrace' request.
*/
struct StackTraceResponseBody
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(stackFrames),
			CLASS_FIELD(totalFrames)
		)
	)
#pragma endregion

	/** 
		The frames of the stackframe. If the array has length zero, there are no stackframes available.
		This means that there is no location information available.
	*/
	std::vector<StackFrame> stackFrames;

	/**
		The total number of frames available in the stack.
		If omitted or if totalFrames is larger than the available frames,
		a client is expected to request frames until a request returns less frames than requested (which indicates the end of the stack).
		Returning monotonically increasing totalFrames values for subsequent requests can be used to enforce paging in the client.
	*/
	std::optional<unsigned> totalFrames;
};

/**
	Arguments for 'scopes' request.
*/
struct ScopesArguments
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(frameId)
		)
	)
#pragma endregion

	/* Retrieve the scopes for this stackframe. */
	unsigned frameId;
};


/**
	A Scope is a named container for variables. Optionally a scope can map to a source or a range within a source.
*/
struct Scope
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(name),
			CLASS_FIELD(presentationHint),
			CLASS_FIELD(variablesReference),
			CLASS_FIELD(namedVariables),
			CLASS_FIELD(indexedVariables),
			CLASS_FIELD(expensive),
			CLASS_FIELD(source),
			CLASS_FIELD(line),
			CLASS_FIELD(column),
			CLASS_FIELD(endLine),
			CLASS_FIELD(endColumn)
		)
	)
#pragma endregion

	/* Name of the scope such as 'Arguments', 'Locals', or 'Registers'. This string is shown in the UI as is and can be translated. */
	std::string name;

	/** An optional hint for how to present this scope in the UI. If this attribute is missing, the scope is shown with a generic UI.
			Values:
			'arguments': Scope contains method arguments.
			'locals': Scope contains local variables.
			'registers': Scope contains registers. Only a single 'registers' scope should be returned from a 'scopes' request.
			etc.
			'arguments' | 'locals' | 'registers' | string;
	*/
	std::optional<std::string> presentationHint;

	/* The variables of this scope can be retrieved by passing the value of variablesReference to the VariablesRequest. */
	unsigned variablesReference;

	/**
		The number of named variables in this scope.
		The client can use this optional information to present the variables in a paged UI and fetch them in chunks.
	*/
	std::optional<unsigned> namedVariables;

	/** The number of indexed variables in this scope.
			The client can use this optional information to present the variables in a paged UI and fetch them in chunks.
	*/
	std::optional<unsigned> indexedVariables;

	/* If true, the number of variables in this scope is large or expensive to retrieve. */
	bool expensive = false;

	/* Optional source for this scope. */
	std::optional<Source> source;

	/* Optional start line of the range covered by this scope. */
	std::optional<unsigned> line;

	/* Optional start column of the range covered by this scope. */
	std::optional<unsigned> column;

	/* Optional end line of the range covered by this scope. */
	std::optional<unsigned> endLine;

	/* Optional end column of the range covered by this scope. */
	std::optional<unsigned> endColumn;


	Scope() = default;

	Scope(std::string_view scopeName, unsigned varRef);
};

/**
	Optional properties of a variable that can be used to determine how to render the variable in the UI.
*/
struct VariablePresentationHint
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(kind),
			CLASS_FIELD(attributes),
			CLASS_FIELD(visibility)
		)
	)
#pragma endregion

	/**
		The kind of variable. Before introducing additional values, try to use the listed values.
			Values:
			'property': Indicates that the object is a property.
			'method': Indicates that the object is a method.
			'class': Indicates that the object is a class.
			'data': Indicates that the object is data.
			'event': Indicates that the object is an event.
			'baseClass': Indicates that the object is a base class.
			'innerClass': Indicates that the object is an inner class.
			'interface': Indicates that the object is an interface.
			'mostDerivedClass': Indicates that the object is the most derived class.
			'virtual': Indicates that the object is virtual, that means it is a synthetic object introducedby the
			adapter for rendering purposes, e.g. an index range for large arrays.
			'dataBreakpoint': Deprecated: Indicates that a data breakpoint is registered for the object. The 'hasDataBreakpoint' attribute should generally be used instead.
			etc.
			'property' | 'method' | 'class' | 'data' | 'event' | 'baseClass' | 'innerClass' | 'interface' | 'mostDerivedClass' | 'virtual' | 'dataBreakpoint' | string;
	*/
	std::optional<std::string> kind;

	/**
		Set of attributes represented as an array of strings. Before introducing additional values, try to use the listed values.
		Values:
		'static': Indicates that the object is static.
		'constant': Indicates that the object is a constant.
		'readOnly': Indicates that the object is read only.
		'rawString': Indicates that the object is a raw string.
		'hasObjectId': Indicates that the object can have an Object ID created for it.
		'canHaveObjectId': Indicates that the object has an Object ID associated with it.
		'hasSideEffects': Indicates that the evaluation had side effects.
		'hasDataBreakpoint': Indicates that the object has its value tracked by a data breakpoint.
		etc.
*/
	std::optional<std::vector<std::string>> attributes;

/** Visibility of variable. Before introducing additional values, try to use the listed values.
		Values: 'public', 'private', 'protected', 'internal', 'final', etc.
*/
	std::optional<std::string> visibility;
};


/**
	A Variable is a name/value pair.
	Optionally a variable can have a 'type' that is shown if space permits or when hovering over the variable's name.
	An optional 'kind' is used to render additional properties of the variable, e.g. different icons can be used to indicate that a variable is public or private.
	If the value is structured (has children), a handle is provided to retrieve the children with the VariablesRequest.
	If the number of named or indexed children is large, the numbers should be returned via the optional 'namedVariables' and 'indexedVariables' attributes.
	The client can use this optional information to present the children in a paged UI and fetch them in chunks.
*/
struct Variable
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(name),
			CLASS_FIELD(value),
			CLASS_FIELD(type),
			CLASS_FIELD(presentationHint),
			CLASS_FIELD(evaluateName),
			CLASS_FIELD(variablesReference),
			CLASS_FIELD(namedVariables),
			CLASS_FIELD(indexedVariables),
			CLASS_FIELD(memoryReference)
		)
	)
#pragma endregion

	/* The variable's name. */
	std::string name;

	/* The variable's value. This can be a multi-line text, e.g. for a function the body of a function. */
	std::string value;

	/** 
		The type of the variable's value. Typically shown in the UI when hovering over the value.
		This attribute should only be returned by a debug adapter if the client has passed the value true for the 'supportsVariableType' capability of the 'initialize' request.
	*/
	std::optional<std::string> type;

	/* Properties of a variable that can be used to determine how to render the variable in the UI. */
	std::optional<VariablePresentationHint> presentationHint;

	/* Optional evaluatable name of this variable which can be passed to the 'EvaluateRequest' to fetch the variable's value. */
	std::optional<std::string> evaluateName;

	/* If variablesReference is > 0, the variable is structured and its children can be retrieved by passing variablesReference to the VariablesRequest. */
	unsigned variablesReference = 0;

	/**
		The number of named child variables.
		The client can use this optional information to present the children in a paged UI and fetch them in chunks.
	*/
	std::optional<unsigned> namedVariables;

	/**
		The number of indexed child variables.
		The client can use this optional information to present the children in a paged UI and fetch them in chunks.
	*/
	std::optional<unsigned> indexedVariables;

	/**
		Optional memory reference for the variable if the variable represents executable code, such as a function pointer.
		This attribute is only required if the client has passed the value true for the 'supportsMemoryReferences' capability of the 'initialize' request.
	*/
	std::optional<std::string> memoryReference;

	Variable() = default;

	Variable(unsigned refId, std::string_view variableName, std::string_view variableValue = {});
};


/* Response to 'scopes' request. */
struct ScopesResponseBody 
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(scopes)
		)
	)
#pragma endregion

	/* The scopes of the stackframe. If the array has length zero, there are no scopes available. */
	std::vector<Scope> scopes;
};


/* Arguments for 'variables' request. */
struct VariablesArguments
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(variablesReference),
			CLASS_FIELD(filter),
			CLASS_FIELD(start),
			CLASS_FIELD(count),
			CLASS_FIELD(format)
		)
	)
#pragma endregion

	/* The Variable reference. */
	unsigned variablesReference;

	/* 
		Optional filter to limit the child variables to either named or indexed. If omitted, both types are fetched.
		'indexed' | 'named'
	*/
	std::string filter;

	/* The index of the first variable to return; if omitted children start at 0. */
	std::optional<unsigned> start;

	/* The number of variables to return. If count is missing or 0, all variables are returned. */
	std::optional<unsigned> count;

	/*
		Specifies details on how to format the Variable values.
		The attribute is only honored by a debug adapter if the capability 'supportsValueFormattingOptions' is true.
	*/
	std::optional<ValueFormat> format;
};


/* Response to 'variables' request. */
struct VariablesResponseBody
{
#pragma region Class info
	CLASS_INFO(
		CLASS_FIELDS(
			CLASS_FIELD(variables)
		)
	)
#pragma endregion

	/* All (or a range) of variables for the given variable reference. */
	std::vector<Variable> variables;
};

} // namespace Runtime::Dap
